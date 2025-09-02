#include "renderer.h"
#include <glad/gl.h>
#include <cmath>

static unsigned int makeShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}
static unsigned int makeProgram(const char* vs, const char* fs) {
    unsigned int v = makeShader(GL_VERTEX_SHADER, vs);
    unsigned int f = makeShader(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

Renderer::Renderer() { initOnce(); }
Renderer::~Renderer() {
    if (tex) glDeleteTextures(1, &tex);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (prog) glDeleteProgram(prog);
}
void Renderer::ensureGL() { if (!prog) initOnce(); }

void Renderer::initOnce() {
    const char* vs = R"(#version 330 core
                        const vec2 verts[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
                        out vec2 uv;
                        void main(){ gl_Position=vec4(verts[gl_VertexID],0,1);
                                     uv = gl_Position.xy*0.5 + 0.5; })";


    const char* fs = R"(#version 330 core
                        in vec2 uv;
                        out vec4 o;
                        uniform usampler2D uTex;
                        uniform vec2 uGrid;   // (w,h)
                        uniform vec2 uView;   // (viewport px)

                        vec3 matColor(uint m){
                            if (m==1u) return vec3(0.85,0.75,0.30); // Sand
                            if (m==2u) return vec3(0.20,0.40,0.90); // Water
                            if (m==3u) return vec3(0.50,0.50,0.55); // Stone
                            return vec3(0.0);
                        }

                        void main(){
                            // pixel-perfect: escala entera y letterbox
                            vec2 scale = floor(uView / uGrid);
                            float s = max(1.0, min(scale.x, scale.y));
                            vec2 size = uGrid * s;
                            vec2 off = (uView - size) * 0.5;
                            vec2 frag = gl_FragCoord.xy - off;
                            if (any(lessThan(frag, vec2(0))) || any(greaterThanEqual(frag, size)))
                                discard;
                            vec2 uv2 = frag / size;
                            uint m = texture(uTex, vec2(uv2.x, 1.0 - uv2.y)).r;

                            if (m==0u) discard;
                            o = vec4(matColor(m),1);
                        }
                        )";
    prog = makeProgram(vs, fs);
    glGenVertexArrays(1, &vao);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Renderer::drawGrid(const std::vector<uint8_t>& indices, int w, int h) {
    ensureGL();

    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (w != texW || h != texH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, indices.data());
        texW = w; texH = h;
    }
    else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, indices.data());
    }

    // viewport actual (para el escalado entero en el FS)
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    glUniform2f(glGetUniformLocation(prog, "uGrid"), (float)w, (float)h);
    glUniform2f(glGetUniformLocation(prog, "uView"), (float)vp[2], (float)vp[3]);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::initPointsOnce() {
    if (pointsProg) return;

    const char* vs = R"(#version 330 core
        layout(location=0) in vec2 aPos;   // NDC
        layout(location=1) in uint aId;    // materialId
        uniform float uPointSize;
        flat out uint vId;
        void main(){
            gl_Position = vec4(aPos,0,1);
            gl_PointSize = uPointSize;
            vId = aId;
        })";

    const char* fs = R"(#version 330 core
        flat in uint vId;
        out vec4 o;
        uniform float uFeather = 1.0;
        layout(std140) uniform Palette { vec4 colors[256]; };
        void main(){
            vec2 p = gl_PointCoord*2.0 - 1.0;
            float r = length(p);
            float alpha = 1.0 - smoothstep(1.0 - (uFeather*0.02), 1.0, r);
            if (alpha <= 0.0) discard;
            o = vec4(colors[vId].rgb, alpha);
        })";

    auto mk = [](GLenum t, const char* src) {
        GLuint s = glCreateShader(t);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
        };
    GLuint v = mk(GL_VERTEX_SHADER, vs);
    GLuint f = mk(GL_FRAGMENT_SHADER, fs);
    pointsProg = glCreateProgram();
    glAttachShader(pointsProg, v); glAttachShader(pointsProg, f); glLinkProgram(pointsProg);
    glDeleteShader(v); glDeleteShader(f);

    glGenVertexArrays(1, &pointsVAO);
    glGenBuffers(1, &pointsVBO);
    glGenBuffers(1, &paletteUBO);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void Renderer::draw(const std::vector<Cell>& cells, int w, int h, bool points, float feather) {
    if (points) {
        drawPoints(cells, w, h, feather);
        return;
    }
    // Construir buffer de índices R8UI y usar drawGrid
    scratch.resize((size_t)w * (size_t)h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            scratch[(size_t)y * (size_t)w + (size_t)x] = cells[(size_t)y * (size_t)w + (size_t)x].m;

    drawGrid(scratch, w, h);
}

void Renderer::drawPoints(const std::vector<Cell>& cells, int w, int h, float feather) {
    initPointsOnce();

    // punto px integer-fit
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    float psX = float(vp[2]) / float(w);
    float psY = float(vp[3]) / float(h);
    float pointSize = std::floor(std::fmin(psX, psY));
    if (pointSize < 1.0f) pointSize = 1.0f;

    struct V { float x, y; uint8_t id; };
    std::vector<V> verts;
    verts.reserve((size_t)w * (size_t)h / 2);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            u8 id = cells[(size_t)y * (size_t)w + (size_t)x].m;
            if (id == (u8)Material::Empty) continue;
            float nx = ((x + 0.5f) / float(w)) * 2.f - 1.f;
            float ny = -(((y + 0.5f) / float(h)) * 2.f - 1.f);
            verts.push_back({ nx, ny, id });
        }
    }

    // paleta desde MatProps
    std::array<GLfloat, 256 * 4> palette{};
    for (int i = 0; i < 256; i++) {
        const MatProps& mp = matProps((u8)i);
        palette[i * 4 + 0] = mp.r / 255.f;
        palette[i * 4 + 1] = mp.g / 255.f;
        palette[i * 4 + 2] = mp.b / 255.f;
        palette[i * 4 + 3] = 1.0f;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, paletteUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(palette), palette.data(), GL_DYNAMIC_DRAW);
    GLuint blockIndex = glGetUniformBlockIndex(pointsProg, "Palette");
    glUniformBlockBinding(pointsProg, blockIndex, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, paletteUBO);

    // subir vértices
    glUseProgram(pointsProg);
    glUniform1f(glGetUniformLocation(pointsProg, "uPointSize"), pointSize);
    glUniform1f(glGetUniformLocation(pointsProg, "uFeather"), feather);

    glBindVertexArray(pointsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, x));
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(V), (void*)offsetof(V, id));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_POINTS, 0, (GLsizei)verts.size());
}
