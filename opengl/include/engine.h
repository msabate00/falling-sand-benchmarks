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

private:
    // Grid + doble buffer
    int w, h;
    std::vector<Cell> front, back;

    // Timestep fijo
    float accumulator = 0.f;
    static constexpr float fixedStep = 1.f / 120.f;
    int parity = 0;

    // Tiles sucios
    static constexpr int TILE = 32;
    int tw = 0, th = 0;
    std::vector<uint8_t> dirty;       // persiste entre frames
    std::vector<uint8_t> nextDirty;   // marcado durante el step
    std::vector<int>     dirtyList;   // tiles a simular este step

    // util
    inline int idx(int x, int y) const { return y * w + x; }
    inline int tidx(int tx, int ty) const { return ty * tw + tx; }
    static inline bool inRange(int x, int y, int W, int H) { return x >= 0 && x < W && y >= 0 && y < H; }
    inline bool isBorder(int x, int y) const { return x <= 0 || x >= w - 1 || y <= 0 || y >= h - 1; }

    // dirty helpers
    void markDirtyTile(int tx, int ty);
    void markDirtyRect(int x0, int y0, int x1, int y1);
    void collectDirty();

    // sim
    void step();
    inline bool tryMove(int sx, int sy, int dx, int dy, const Cell& c);
    void swapBuffers() { front.swap(back); }
};
