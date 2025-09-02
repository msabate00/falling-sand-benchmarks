#pragma once
#include <vector>
#include <cstdint>
#include "material.h"

class Engine {
public:
    Engine(int gridW, int gridH);

    void update(float dt);
    void draw();                          // GL_POINTS redondos
    void paint(int cx, int cy, Material m, int radius);

    int width()  const { return w; }
    int height() const { return h; }

    // util
    int idx(int x, int y) const { return y * w + x; }
    static bool inRange(int x, int y, int W, int H) { return x >= 0 && x < W && y >= 0 && y < H; }
    bool inRange(int x, int y) { return x >= 0 && x < w && y >= 0 && y < h; }
    Cell read(int x, int y) { 
        return (inRange(x, y)) ? front[idx(x, y)] : Cell{ (u8)Material::NullCell };
        
    }

    static bool randbit(int x, int y, int parity) {
        uint32_t h = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ (uint32_t)(parity * 0x9E3779B9u);
        h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
        return (h & 1u) != 0u;
    }

    bool tryMove(int sx, int sy, int dx, int dy, const Cell& c);
    bool trySwap(int sx, int sy, int dx, int dy, const Cell& c);

    void setCell(int x, int y, u8 m);


    bool stepOnce = false;
    bool paused = false;

private:
    // Grid + doble buffer
    int w, h;
    std::vector<Cell> front, back;

    // Timestep fijo
    float accumulator = 0.f;
    static constexpr float fixedStep = 1.f / 120.f;
    int parity = 0;

    

    // sim
    void step();
    void swapBuffers() { front.swap(back); }

    
};
