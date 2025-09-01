#include "engine.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <glad/gl.h>

// ---------------------------- ctor ----------------------------
Engine::Engine(int gridW, int gridH) : w(gridW), h(gridH) {
    front.assign(w * h, Cell{ Material::Empty,0 });
    back.assign(w * h, Cell{ Material::Empty,0 });

    tw = (w + TILE - 1) / TILE;
    th = (h + TILE - 1) / TILE;
    dirty.assign(tw * th, 1);            // todo sucio el primer frame
    nextDirty.assign(tw * th, 0);
}

// ----------------------- dirty helpers ------------------------
void Engine::markDirtyTile(int tx, int ty) {
    if (tx < 0 || tx >= tw || ty < 0 || ty >= th) return;
    nextDirty[tidx(tx, ty)] = 1;
}
void Engine::markDirtyRect(int x0, int y0, int x1, int y1) {
    x0 = std::max(0, std::min(x0, w - 1));
    y0 = std::max(0, std::min(y0, h - 1));
    x1 = std::max(0, std::min(x1, w - 1));
    y1 = std::max(0, std::min(y1, h - 1));
    int tx0 = x0 / TILE, ty0 = y0 / TILE;
    int tx1 = x1 / TILE, ty1 = y1 / TILE;
    for (int ty = ty0; ty <= ty1; ++ty)
        for (int tx = tx0; tx <= tx1; ++tx)
            markDirtyTile(tx, ty);
    // margen de propagación a tiles vecinos
    for (int ty = ty0 - 1; ty <= ty1 + 1; ++ty) { markDirtyTile(tx0 - 1, ty); markDirtyTile(tx1 + 1, ty); }
    for (int tx = tx0 - 1; tx <= tx1 + 1; ++tx) { markDirtyTile(tx, ty0 - 1); markDirtyTile(tx, ty1 + 1); }
}
void Engine::collectDirty() {
    dirtyList.clear();
    for (int t = 0; t < tw * th; ++t) {
        if (dirty[t] || nextDirty[t]) { dirtyList.push_back(t); dirty[t] = 0; }
    }
    std::fill(nextDirty.begin(), nextDirty.end(), 0);
    if (dirtyList.empty()) for (int t = 0; t < tw * th; ++t) dirtyList.push_back(t);
}

// ---------------------------- sim -----------------------------
void Engine::update(float dt) {
    accumulator += dt;
    while (accumulator >= fixedStep) {
        back = front;
        collectDirty();
        step();
        swapBuffers();
        accumulator -= fixedStep;
        parity ^= 1;
    }
}

inline bool Engine::tryMove(int sx, int sy, int dx, int dy, const Cell& c) {
    int nx = sx + dx, ny = sy + dy;
    if (!inRange(nx, ny, w, h)) return false;
    if (isBorder(nx, ny))      return false; // paredes invisibles en el marco

    int si = idx(sx, sy), ni = idx(nx, ny);
    if (front[ni].m == Material::Empty) {
        back[ni] = c; back[si].m = Material::Empty;
        int minx = std::min(sx, nx) - 1, maxx = std::max(sx, nx) + 1;
        int miny = std::min(sy, ny) - 1, maxy = std::max(sy, ny) + 1;
        markDirtyRect(minx, miny, maxx, maxy);
        return true;
    }
    return false;
}

void Engine::step() {
    auto M = [&](int x, int y) { return front[idx(x, y)].m; };

    for (int t : dirtyList) {
        int tx = t % tw, ty = t / tw;
        int xBegin = tx * TILE;
        int yBegin = ty * TILE;
        int xEnd = std::min(xBegin + TILE, w);
        int yEnd = std::min(yBegin + TILE, h);

        for (int y = yEnd - 1; y >= yBegin; --y) {           // incluye última fila del tile
            bool l2r = ((y ^ parity) & 1);
            int x0 = l2r ? xBegin : (xEnd - 1);
            int x1 = l2r ? xEnd : (xBegin - 1);
            int s = l2r ? 1 : -1;

            for (int x = x0; x != x1; x += s) {
                const Cell c = front[idx(x, y)];
                switch (c.m) {
                case Material::Sand: {
                    if (tryMove(x, y, 0, +1, c)) break;                // caer
                    bool leftFirst = ((x + y + parity) & 1) == 0;   // rampas
                    int dxa = leftFirst ? -1 : +1, dxb = -dxa;
                    if (tryMove(x, y, dxa, +1, c)) break;
                    if (tryMove(x, y, dxb, +1, c)) break;
                } break;

                case Material::Water: {
                    // Igual a tu versión de Python: abajo, si no puede -> 1 paso lateral
                    if (tryMove(x, y, 0, +1, c)) break;
                    // apilar: si debajo hay agua, no te vayas lateral
                    if (y + 1 < h && M(x, y + 1) == Material::Water) break;
                    int dir = ((x + y + parity) & 1) ? -1 : +1;
                    if (tryMove(x, y, dir, 0, c)) break;
                    if (tryMove(x, y, -dir, 0, c)) break;
                } break;

                default: break;
                }
            }
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
            if (dx * dx + dy * dy <= r2) front[idx(x, y)].m = m;
        }
    markDirtyRect(xmin, ymin, xmax, ymax);
}

// --------------------------- draw() ---------------------------
void Engine::draw() {
    static GLuint prog = 0, vao = 0, vbo = 0;

    auto checkShader = [](GLuint s, const char* name) {
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0'); glGetShaderInfoLog(s, len, nullptr, log.data());
            fprintf(stderr, "[GL] Shader compile error (%s):\n%s\n", name, log.c_str());
        }
        return ok != 0;
        };
    auto checkProgram = [](GLuint p) {
        GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0'); glGetProgramInfoLog(p, len, nullptr, log.data());
            fprintf(stderr, "[GL] Program link error:\n%s\n", log.c_str());
        }
        return ok != 0;
        };

    if (!prog) {
        const char* vs = R"(#version 330 core
            layout(location=0) in vec2 aPos;   // NDC
            uniform float uPointSize;
            void main(){ gl_Position=vec4(aPos,0,1); gl_PointSize=uPointSize; })";

        // puntos redondos con borde suave (usa blending)
        const char* fs = R"(#version 330 core
            out vec4 o; uniform vec3 uColor; uniform float uFeather = 1.0;
            void main(){
                vec2 p = gl_PointCoord*2.0 - 1.0; // centro (0,0)
                float r = length(p);
                float alpha = 1.0 - smoothstep(1.0 - (uFeather*0.02), 1.0, r);
                if (alpha <= 0.0) discard;
                o = vec4(uColor, alpha);
            })";

        auto mk = [&](GLenum t, const char* src, const char* name) {
            GLuint s = glCreateShader(t); glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
            checkShader(s, name); return s;
            };
        GLuint v = mk(GL_VERTEX_SHADER, vs, "VS(points)");
        GLuint f = mk(GL_FRAGMENT_SHADER, fs, "FS(points)");
        prog = glCreateProgram();
        glAttachShader(prog, v); glAttachShader(prog, f); glLinkProgram(prog);
        checkProgram(prog);
        glDeleteShader(v); glDeleteShader(f);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
    }

    // tamaño entero del punto para evitar bandas
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    float psX = float(vp[2]) / float(w);
    float psY = float(vp[3]) / float(h);
    float pointSize = std::floor(std::fmin(psX, psY));
    if (pointSize < 1.0f) pointSize = 1.0f;

    // posiciones por material (centro celda -> NDC, Y invertida)
    std::vector<float> sand, water, stone;
    sand.reserve(w * h / 4); water.reserve(w * h / 4); stone.reserve(w * h / 4);
    auto push = [&](std::vector<float>& v, int x, int y) {
        float nx = ((x + 0.5f) / float(w)) * 2.f - 1.f;
        float ny = ((y + 0.5f) / float(h)) * 2.f - 1.f;
        ny = -ny;
        v.push_back(nx); v.push_back(ny);
        };
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            switch (front[idx(x, y)].m) {
            case Material::Sand:  push(sand, x, y);  break;
            case Material::Water: push(water, x, y); break;
            case Material::Stone: push(stone, x, y); break;
            default: break;
            }
        }
    }

    auto drawPoints = [&](const std::vector<float>& v, float r, float g, float b) {
        if (v.empty()) return;
        glUseProgram(prog);
        glUniform1f(glGetUniformLocation(prog, "uPointSize"), pointSize);
        glUniform1f(glGetUniformLocation(prog, "uFeather"), 1.0f); // borde suave
        glUniform3f(glGetUniformLocation(prog, "uColor"), r, g, b);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size() * sizeof(float)), v.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_POINTS, 0, (GLsizei)(v.size() / 2));
        };

    drawPoints(stone, 0.50f, 0.50f, 0.55f);
    drawPoints(sand, 0.85f, 0.75f, 0.30f);
    drawPoints(water, 0.20f, 0.40f, 0.90f);
}
