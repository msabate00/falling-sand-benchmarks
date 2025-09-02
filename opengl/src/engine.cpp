#include "engine.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

using std::uint8_t;

// ---------------------------- util ----------------------------
bool Engine::randbit(int x, int y, int parity) {
    uint32_t h = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ (uint32_t)(parity * 0x9E3779B9u);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    return (h & 1u) != 0u;
}

// ---------------------------- ctor ----------------------------
Engine::Engine(int gridW, int gridH) : w(gridW), h(gridH) {
    front.assign(w * h, Cell{ (u8)Material::Empty,0 });
    back.assign(w * h, Cell{ (u8)Material::Empty,0 });
    mFront.assign(w * h, (u8)Material::Empty);
    mBack.assign(w * h, (u8)Material::Empty);
    registerDefaultMaterials();
    // Dirty-rect: forzar upload completo inicial
    clearDirty();
    markDirtyRect(0, 0, w - 1, h - 1);
}

// ---------------------- dirty helpers -------------------------
void Engine::clearDirty() {
    dirtyMinX = w; dirtyMinY = h;
    dirtyMaxX = -1; dirtyMaxY = -1;
}
void Engine::markDirty(int x, int y) {
    if (!inRange(x, y)) return;
    if (x < dirtyMinX) dirtyMinX = x;
    if (y < dirtyMinY) dirtyMinY = y;
    if (x > dirtyMaxX) dirtyMaxX = x;
    if (y > dirtyMaxY) dirtyMaxY = y;
}
void Engine::markDirtyRect(int x0, int y0, int x1, int y1) {
    x0 = std::max(0, std::min(x0, w - 1));
    y0 = std::max(0, std::min(y0, h - 1));
    x1 = std::max(0, std::min(x1, w - 1));
    y1 = std::max(0, std::min(y1, h - 1));
    if (x1 < x0 || y1 < y0) return;
    if (x0 < dirtyMinX) dirtyMinX = x0;
    if (y0 < dirtyMinY) dirtyMinY = y0;
    if (x1 > dirtyMaxX) dirtyMaxX = x1;
    if (y1 > dirtyMaxY) dirtyMaxY = y1;
}
bool Engine::takeDirtyRect(int& x, int& y, int& rw, int& rh) {
    if (dirtyMaxX < dirtyMinX || dirtyMaxY < dirtyMinY) { x = y = rw = rh = 0; return false; }
    x = dirtyMinX; y = dirtyMinY;
    rw = dirtyMaxX - dirtyMinX + 1;
    rh = dirtyMaxY - dirtyMinY + 1;
    clearDirty();
    return true;
}

// ---------------------------- sim -----------------------------
void Engine::update(float dt) {
    accumulator += dt;
    while (accumulator >= fixedStep && (!paused || stepOnce)) {
        // back = front; y SoA
        back = front;
        mBack = mFront;

        step();

        swapBuffers();
        accumulator -= fixedStep;
        parity ^= 1;

        if (paused) { stepOnce = false; break; }
    }
}

bool Engine::tryMove(int sx, int sy, int dx, int dy, const Cell& c) {
    int nx = sx + dx, ny = sy + dy;
    if (!inRange(nx, ny)) return false;
    int si = idx(sx, sy), ni = idx(nx, ny);

    if (back[ni].m != (u8)Material::Empty) return false;

    back[ni] = c;
    if (back[si].m == front[si].m) back[si].m = (u8)Material::Empty;

    // SoA + dirty
    mBack[ni] = c.m;
    mBack[si] = (u8)Material::Empty;
    markDirty(sx, sy);
    markDirty(nx, ny);
    return true;
}

bool Engine::trySwap(int sx, int sy, int dx, int dy, const Cell& c) {
    int nx = sx + dx, ny = sy + dy;
    if (!inRange(nx, ny, w, h)) return false;

    int si = idx(sx, sy);
    int ni = idx(nx, ny);
    if (si == ni) return false;

    const Cell& dst = front[ni];

    back[ni] = c;
    back[si] = dst;

    // SoA + dirty
    mBack[ni] = c.m;
    mBack[si] = dst.m;
    markDirty(sx, sy);
    markDirty(nx, ny);
    return true;
}

void Engine::setCell(int x, int y, u8 m) {
    if (!inRange(x, y)) return;
    int i = idx(x, y);
    back[i].m = m;
    mBack[i] = m;
    markDirty(x, y);
}

void Engine::step() {
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
            if (dx * dx + dy * dy <= r2) {
                int i = idx(x, y);
                front[i].m = (u8)m;   // efecto inmediato
                mFront[i] = (u8)m;    // SoA inmediato
                markDirty(x, y);
            }
        }
    markDirtyRect(xmin, ymin, xmax, ymax);
}
