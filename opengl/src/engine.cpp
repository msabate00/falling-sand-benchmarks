#include "engine.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

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
