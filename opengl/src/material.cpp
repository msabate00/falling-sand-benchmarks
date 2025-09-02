#include <array>
#include "material.h"
#include "engine.h"

static std::array<MatProps, 256> g_mat{};

const MatProps& matProps(u8 id) { return g_mat[id]; }

static void SandUpdate(Engine& E, int x, int y, const Cell& self) {

    if (E.tryMove(x, y, 0, +1, self)) return; // caer


    if (E.inRange(x, y + 1) && E.read(x, y + 1).m == (u8)Material::Water && E.trySwap(x, y, 0, +1, self)) return;

    bool leftFirst = !Engine::randbit(x, y, 0);
    int da = leftFirst ? -1 : +1, db = -da;

    if ((Material)E.read(x + da, y + 1).m == Material::Water && E.trySwap(x, y, da, +1, self)) return;
    if ((Material)E.read(x + db, y + 1).m == Material::Water && E.trySwap(x, y, db, +1, self)) return;

    if (E.tryMove(x, y, da, +1, self)) return;
    E.tryMove(x, y, db, +1, self);
}

static void WaterUpdate(Engine& E, int x, int y, const Cell& self) {
    if (E.tryMove(x, y, 0, +1, self)) return;
    bool leftFirst = !Engine::randbit(x, y, 1);
    int da = leftFirst ? -1 : +1, db = -da;

    if ((Material)E.read(x + da, y + 1).m == Material::Empty && E.tryMove(x, y, da, +1, self)) return;
    if ((Material)E.read(x + db, y + 1).m == Material::Empty && E.tryMove(x, y, db, +1, self)) return;

    if ((Material)E.read(x + da, y).m == Material::Empty && E.tryMove(x, y, da, 0, self)) return;
    E.tryMove(x, y, db, 0, self);
}

static void WoodUpdate(Engine& E, int x, int y, const Cell& self) {
    bool burn = false;
    for (int dy = -1; dy <= 1 && !burn; dy++) {
        for (int dx = -1; dx <= 1 && !burn; dx++) {
            if ((dx != 0 || dy != 0) && E.inRange(x + dx, y + dy)) {
                if (E.read(x + dx, y + dy).m == (u8)Material::Fire) {
                    burn = true;
                }
            }
        }
    }
    if (burn) {
        E.setCell(x, y, (u8)Material::Fire);
    }
}

static void FireUpdate(Engine& E, int x, int y, const Cell& self) {
    if ((rand() % 100) < 5) {
        E.setCell(x, y, (u8)Material::Empty);
        return;
    }

    if (E.inRange(x, y - 1) && E.read(x, y-1).m == (u8)Material::Empty) {
        if ((rand() % 100) < 20) {
            E.setCell(x, y, (u8)Material::Smoke);
        }
    }

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if ((dx != 0 || dy != 0) && E.inRange(x + dx, y + dy)) {
                if (E.read(x + dx, y + dy).m  == (u8)Material::Wood) {
                    E.setCell(x, y, (u8)Material::Fire);
                }
            }
        }
    }
}

static void SmokeUpdate(Engine& E, int x, int y, const Cell& self) {

    if (E.tryMove(x, y, 0, -1, self)) return;


    bool leftFirst = !E.randbit(x, y, 0);
    int dxa = leftFirst ? -1 : +1, dxb = -dxa;

    if (E.tryMove(x, y, dxa, -1, self)) return;
    if (E.tryMove(x, y, dxb, -1, self)) return;

    if ((rand() % 100) < 2) {
        E.setCell(x, y, (u8)Material::Empty);
    }
}

static void StoneUpdate(Engine&, int, int, const Cell&) { /* inmóvil */ }

void registerDefaultMaterials() {

    //MatProp                           //NAME      //Color             //Densidad
    g_mat[(u8)Material::Empty] =    {   "Empty",    0,0,0,0,           0,      nullptr };
    g_mat[(u8)Material::Sand] =     {   "Sand",     217,191,77,255,    3,      &SandUpdate };
    g_mat[(u8)Material::Water] =    {   "Water",    51,102,230,200,    1,      &WaterUpdate };
    g_mat[(u8)Material::Stone] =    {   "Stone",    128,128,140,255,   255,    &StoneUpdate };
    g_mat[(u8)Material::Wood] =     {   "Wood",     142,86,55,255,     255,    &WoodUpdate };
    g_mat[(u8)Material::Fire] =     {   "Fire",     255,35,1,255,      255,    &FireUpdate };
    g_mat[(u8)Material::Smoke] =    {   "Smoke",    28,13,2,255,       255,    &SmokeUpdate };
}