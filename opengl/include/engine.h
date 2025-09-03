#pragma once
#include <vector>
#include <cstdint>
#include "material.h"

class Engine {
public:
    Engine(int gridW, int gridH);

    void update(float dt);
    void paint(int cx, int cy, Material m, int radius);

    int width()  const { return w; }
    int height() const { return h; }

    const std::uint8_t* planeM() const { return mFront.data(); }

    // Dirty-rect: true si hay cambios (rellena x,y,rw,rh)
    bool takeDirtyRect(int& x, int& y, int& rw, int& rh);

    // util
    int idx(int x, int y) const { return y * w + x; }
    static bool inRange(int x, int y, int W, int H) { return x >= 0 && x < W && y >= 0 && y < H; }
    bool inRange(int x, int y) { return x >= 0 && x < w && y >= 0 && y < h; }
    Cell read(int x, int y) {
        return (inRange(x, y)) ? front[idx(x, y)] : Cell{ (u8)Material::NullCell };
    }

    static bool randbit(int x, int y, int parity);

    bool tryMove(int sx, int sy, int dx, int dy, const Cell& c);
    bool trySwap(int sx, int sy, int dx, int dy, const Cell& c);

    void setCell(int x, int y, u8 m);

    bool stepOnce = false;
    bool paused = false;

private:

    int w, h;
    std::vector<Cell> front, back;
    std::vector<u8> mFront, mBack;

    // Timestep fijo
    float accumulator = 0.f;
    static constexpr float fixedStep = 1.f / 120.f;
    int parity = 0;

    // sim
    void step();
    void swapBuffers() { front.swap(back); mFront.swap(mBack); }

    // Dirty tracking
    int dirtyMinX, dirtyMinY, dirtyMaxX, dirtyMaxY;
    void clearDirty();
    void markDirty(int x, int y);
    void markDirtyRect(int x0, int y0, int x1, int y1);
};
