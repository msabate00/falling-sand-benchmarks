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
                        const vec2 V[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
                        out vec2 uv;
                        void main(){
                            gl_Position=vec4(V[gl_VertexID],0,1);
                            uv = gl_Position.xy*0.5 + 0.5;
                        })";

    const char* fs = R"(#version 330 core
                        in vec2 uv;
                        out vec4 o;
                        uniform usampler2D uTex; // índices
                        uniform vec2 uGrid;      // (w,h)
                        uniform vec2 uView;      // viewport px

                        layout(std140) uniform Palette { vec4 colors[256]; };

                        void main(){
                            // escalado entero con letterbox
                            vec2 scale = floor(uView / uGrid);
                            float s = max(1.0, min(scale.x, scale.y));
                            vec2 size = uGrid * s;
                            vec2 off  = (uView - size) * 0.5;

                            vec2 frag = gl_FragCoord.xy - off;
                            if (any(lessThan(frag, vec2(0))) || any(greaterThanEqual(frag, size)))
                                discard;

                            vec2 uv2 = frag / size;
                            uint m = texture(uTex, vec2(uv2.x, 1.0 - uv2.y)).r;

                            if (m==0u) discard;                // Empty
                            vec4 c = colors[int(m)];           // RGBA desde UBO
                            if (c.a <= 0.0) discard;
                            o = c;
                        })";

    prog = makeProgram(vs, fs);

    glGenVertexArrays(1, &vao);
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    loc_uTex = glGetUniformLocation(prog, "uTex");
    loc_uGrid = glGetUniformLocation(prog, "uGrid");
    loc_uView = glGetUniformLocation(prog, "uView");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenBuffers(1, &paletteUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, paletteUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4 * 256, nullptr, GL_STATIC_DRAW);

    std::vector<float> pal(4 * 256, 0.0f);
    for (int i = 0; i < 256; ++i) {
        const MatProps& mp = matProps((u8)i);
        pal[4 * i + 0] = mp.r / 255.0f;
        pal[4 * i + 1] = mp.g / 255.0f;
        pal[4 * i + 2] = mp.b / 255.0f;
        pal[4 * i + 3] = mp.a / 255.0f;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, pal.size() * sizeof(float), pal.data());

    GLuint blk = glGetUniformBlockIndex(prog, "Palette");
    glUniformBlockBinding(prog, blk, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, paletteUBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void Renderer::drawGrid(const std::vector<uint8_t>& indices, int w, int h, int viewW, int viewH) {
    ensureGL();

    glBindTexture(GL_TEXTURE_2D, tex);

    if (w != texW || h != texH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = w; texH = h;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, indices.data());

    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(loc_uTex, 0);
    glUniform2f(loc_uGrid, float(w), float(h));
    glUniform2f(loc_uView, float(viewW), float(viewH));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::draw(const std::vector<Cell>& cells, int w, int h, int viewW, int viewH) {

    scratch.resize(size_t(w) * size_t(h));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            scratch[size_t(y) * size_t(w) + size_t(x)] =
            cells[size_t(y) * size_t(w) + size_t(x)].m;

    drawGrid(scratch, w, h, viewW, viewH);
}
