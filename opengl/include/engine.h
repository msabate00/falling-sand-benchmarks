#pragma once
#include <vector>
#include "material.h"

class Engine {
public:
    Engine(int gridW, int gridH);

    // dt en segundos; internamente hacemos paso fijo
    void update(float dt);
    void draw() const;

    // herramientas editor
    void paint(int cx, int cy, Material m, int radius);

    // utilidades
    int width()  const { return w; }
    int height() const { return h; }

private:
    int w, h;
    float accumulator = 0.0f;
    static constexpr float fixedStep = 1.0f / 120.0f;

    std::vector<Cell> front, back;

    inline int idx(int x, int y) const { return y * w + x; }
    void swapBuffers() { front.swap(back); }
    void step(int parity); // una iteración de reglas (parity alterna 0/1)
};
