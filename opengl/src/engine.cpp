#include "engine.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <glad/gl.h>

// ---------------------------- ctor ----------------------------
Engine::Engine(int gridW, int gridH) : w(gridW), h(gridH) {
    front.assign(w * h, Cell{ (u8)Material::Empty,0 });
    back.assign(w * h, Cell{ (u8)Material::Empty,0 });
    registerDefaultMaterials();
}

// ---------------------------- sim -----------------------------
void Engine::update(float dt) {
    accumulator += dt;
    while (accumulator >= fixedStep && (!paused || stepOnce)) {
        back = front;

        step();

        swapBuffers();
        accumulator -= fixedStep;
        parity ^= 1;

        if (paused) {
            stepOnce = false;
            break;
        }
    }
}

bool Engine::tryMove(int sx, int sy, int dx, int dy, const Cell& c) {
    int nx = sx + dx, ny = sy + dy;
    if (!inRange(nx, ny)) return false;
    int si = idx(sx, sy), ni = idx(nx, ny);

    if (back[ni].m != (u8)Material::Empty) return false;

    back[ni] = c;
    if (back[si].m == front[si].m) back[si].m = (u8)Material::Empty;
    return true;
}

bool Engine::trySwap(int sx, int sy, int dx, int dy, const Cell& c) {
    int nx = sx + dx;
    int ny = sy + dy;
    if (!inRange(nx, ny, w, h)) return false;

    int si = idx(sx, sy);
    int ni = idx(nx, ny);
    if (si == ni) return false;

    const Cell& dst = front[ni];

    back[ni] = c;
    back[si] = dst;
    // no dirty-marking
    return true;
}

void Engine::setCell(int x, int y, u8 m) {
    back[idx(x, y)].m = (u8)m;
}

void Engine::step() {
    auto M = [&](int x, int y) { return front[idx(x, y)].m; };

    for (int y = h - 1; y >= 0; --y) {
        bool l2r = ((y ^ parity) & 1);
        int x0 = l2r ? 0 : (w - 1);
        int x1 = l2r ? w : -1;
        int s = l2r ? 1 : -1;

        for (int x = x0; x != x1; x += s) {
            const Cell c = front[idx(x, y)];
            if (c.m == (u8)Material::Empty) continue;
            const MatProps& mp = matProps(c.m);
            if (mp.update) mp.update(*this, x, y, c);
        }
    }
}

// --------------------------- pintar ---------------------------
void Engine::paint(int cx, int cy, Material m, int r) {
    int r2 = r * r;
    int xmin = std::max(0, cx - r), xmax = std::min(w - 1, cx + r);
    int ymin = std::max(0, cy - r), ymax = std::min(h - 1, cy + r);
    for (int y = ymin; y <= ymax; ++y)
        for (int x = xmin; x <= xmax; ++x) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r2) front[idx(x, y)].m = (u8)m;
        }
    // no dirty-marking
}

// --------------------------- draw() ---------------------------
void Engine::draw() {
    static GLuint prog = 0, vao = 0, vbo = 0;

    if (!prog) {
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

            // paleta cargada desde CPU (array de vec4)
            layout(std140) uniform Palette {
                vec4 colors[256];
            };

            void main(){
                vec2 p = gl_PointCoord*2.0 - 1.0;
                float r = length(p);
                float alpha = 1.0 - smoothstep(1.0 - (uFeather*0.02), 1.0, r);
                if (alpha <= 0.0) discard;
                o = vec4(colors[vId].rgb, alpha);
            })";

        auto mk = [&](GLenum t, const char* src, const char* name) {
            GLuint s = glCreateShader(t);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) { /* log omitted for brevity */ }
            return s;
            };
        GLuint v = mk(GL_VERTEX_SHADER, vs, "VS(points)");
        GLuint f = mk(GL_FRAGMENT_SHADER, fs, "FS(points)");
        prog = glCreateProgram();
        glAttachShader(prog, v); glAttachShader(prog, f); glLinkProgram(prog);
        glDeleteShader(v); glDeleteShader(f);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
    }

    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    float psX = float(vp[2]) / float(w);
    float psY = float(vp[3]) / float(h);
    float pointSize = std::floor(std::fmin(psX, psY));
    if (pointSize < 1.0f) pointSize = 1.0f;

    struct V { float x, y; uint8_t id; };
    std::vector<V> verts; verts.reserve(w * h / 2);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            u8 id = front[idx(x, y)].m;
            if (id == (u8)Material::Empty) continue;
            float nx = ((x + 0.5f) / float(w)) * 2.f - 1.f;
            float ny = -(((y + 0.5f) / float(h)) * 2.f - 1.f);
            verts.push_back({ nx, ny, id });
        }
    }

    // subir paleta de colores
    std::array<GLfloat, 256 * 4> palette{};
    for (int i = 0; i < 256; i++) {
        const MatProps& mp = matProps((u8)i);
        palette[i * 4 + 0] = mp.r / 255.f;
        palette[i * 4 + 1] = mp.g / 255.f;
        palette[i * 4 + 2] = mp.b / 255.f;
        palette[i * 4 + 3] = 1.0f;
    }
    GLuint ubo; static GLuint paletteUBO = 0;
    if (!paletteUBO) glGenBuffers(1, &paletteUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, paletteUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(palette), palette.data(), GL_DYNAMIC_DRAW);
    GLuint blockIndex = glGetUniformBlockIndex(prog, "Palette");
    glUniformBlockBinding(prog, blockIndex, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, paletteUBO);

    // subir vértices
    glUseProgram(prog);
    glUniform1f(glGetUniformLocation(prog, "uPointSize"), pointSize);
    glUniform1f(glGetUniformLocation(prog, "uFeather"), 1.0f);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, x));
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(V), (void*)offsetof(V, id));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_POINTS, 0, (GLsizei)verts.size());
}
