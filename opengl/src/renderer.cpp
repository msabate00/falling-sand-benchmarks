#include "renderer.h"
#include <glad/gl.h>

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
