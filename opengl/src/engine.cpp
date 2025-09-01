#include "engine.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

// --- helpers ---
static inline bool inRange(int x, int y, int w, int h) {
    return (x >= 0 && x < w && y >= 0 && y < h);
}

// --- util shader ---
static GLuint makeShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    return s;
}
static GLuint makeProgram(const char* vs, const char* fs) {
    GLuint v = makeShader(GL_VERTEX_SHADER, vs);
    GLuint f = makeShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

Engine::Engine(int gridW, int gridH) : w(gridW), h(gridH) {
    front.resize(w * h);
    back.resize(w * h);
    std::fill(front.begin(), front.end(), Cell{ Material::Empty, 0 });
    std::fill(back.begin(), back.end(), Cell{ Material::Empty, 0 });
}

void Engine::update(float dt) {
    accumulator += dt;
    static int parity = 0;
    while (accumulator >= fixedStep) {
        // copia estado actual a back para empezar “limpio”
        back = front;

        step(parity);
        swapBuffers();
        accumulator -= fixedStep;
        parity ^= 1;
    }
}

void Engine::step(int parity) {
    // barrido de abajo->arriba; alternar izquierda->derecha / derecha->izquierda
    for (int y = h - 2; y >= 0; --y) {
        if ((y ^ parity) & 1) {
            for (int x = 0; x < w; ++x) {
                const int i = idx(x, y);
                Cell c = front[i];
                if (c.m == Material::Sand) {
                    int below = idx(x, y + 1);
                    if (front[below].m == Material::Empty) {
                        back[below] = c; back[i].m = Material::Empty; continue;
                    }
                    // diagonal aleatoria via paridad del x
                    bool leftFirst = ((x + y + parity) & 1) == 0;
                    int dx1 = leftFirst ? -1 : 1;
                    int dx2 = leftFirst ? 1 : -1;

                    int i1x = x + dx1, i2x = x + dx2;
                    if (inRange(i1x, y + 1, w, h) && front[idx(i1x, y + 1)].m == Material::Empty) {
                        back[idx(i1x, y + 1)] = c; back[i].m = Material::Empty; continue;
                    }
                    if (inRange(i2x, y + 1, w, h) && front[idx(i2x, y + 1)].m == Material::Empty) {
                        back[idx(i2x, y + 1)] = c; back[i].m = Material::Empty; continue;
                    }
                    // si no puede moverse, se queda; back ya tiene la copia
                }
                // otros materiales: por ahora, inertes
            }
        }
        else {
            for (int x = w - 1; x >= 0; --x) {
                const int i = idx(x, y);
                Cell c = front[i];
                if (c.m == Material::Sand) {
                    int below = idx(x, y + 1);
                    if (front[below].m == Material::Empty) {
                        back[below] = c; back[i].m = Material::Empty; continue;
                    }
                    bool leftFirst = ((x + y + parity) & 1) == 0;
                    int dx1 = leftFirst ? -1 : 1;
                    int dx2 = leftFirst ? 1 : -1;

                    int i1x = x + dx1, i2x = x + dx2;
                    if (inRange(i1x, y + 1, w, h) && front[idx(i1x, y + 1)].m == Material::Empty) {
                        back[idx(i1x, y + 1)] = c; back[i].m = Material::Empty; continue;
                    }
                    if (inRange(i2x, y + 1, w, h) && front[idx(i2x, y + 1)].m == Material::Empty) {
                        back[idx(i2x, y + 1)] = c; back[i].m = Material::Empty; continue;
                    }
                }
            }
        }
    }
}

// Pintar círculo en grid
void Engine::paint(int cx, int cy, Material m, int radius) {
    int r2 = radius * radius;
    int xmin = std::max(0, cx - radius);
    int xmax = std::min(w - 1, cx + radius);
    int ymin = std::max(0, cy - radius);
    int ymax = std::min(h - 1, cy + radius);
    for (int y = ymin; y <= ymax; ++y) {
        for (int x = xmin; x <= xmax; ++x) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r2) {
                front[idx(x, y)].m = m;
            }
        }
    }
}

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Mapea celda -> NDC y pinta como punto
void Engine::draw() const {
    static GLuint prog = 0, vao = 0, vbo = 0;
    if (!prog) {
        const char* vs = R"(#version 330 core
        layout(location=0) in vec2 aPos;
        uniform float uPointSize = 2.0;
        void main(){
            gl_Position = vec4(aPos.x, -aPos.y, 0.0, 1.0);
            gl_PointSize = uPointSize;
        })";
        const char* fs = R"(#version 330 core
        out vec4 FragColor;
        uniform vec3 uColor = vec3(0.9,0.8,0.3);
        void main(){ FragColor = vec4(uColor,1.0); })";
        prog = makeProgram(vs, fs);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glEnable(GL_PROGRAM_POINT_SIZE);
    }

    // construir posiciones NDC de celdas no vacías
    std::vector<float> pos; pos.reserve(w * h / 2);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Cell& c = front[idx(x, y)];
            if (c.m == Material::Empty) continue;
            float nx = ((x + 0.5f) / float(w)) * 2.f - 1.f;
            float ny = ((y + 0.5f) / float(h)) * 2.f - 1.f;
            pos.push_back(nx); pos.push_back(ny);
        }
    }

    glUseProgram(prog);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), pos.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color único temporal
    GLint uColor = glGetUniformLocation(prog, "uColor");
    glUniform3f(uColor, 0.85f, 0.75f, 0.30f);

    glDrawArrays(GL_POINTS, 0, (GLsizei)(pos.size() / 2));
}
