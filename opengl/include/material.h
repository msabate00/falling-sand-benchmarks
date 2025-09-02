#pragma once
#include <cstdint>
#include <array>
#include <string_view>

struct Engine;

using u8 = std::uint8_t;

enum class Material : u8 { NullCell = -1, Empty = 0, Sand, Water, Stone, Wood, Fire, Smoke };

struct Cell {
    u8 m = (u8)Material::Empty;
    u8  meta = 0;
    int vx = 0, vy = 0;
};

struct MatProps {
    std::string_view name;
    // color RGBA simple
    u8 r = 0, g = 0, b = 0, a = 255;
    u8 density = 1;
    // comportamiento
    void (*update)(Engine&, int x, int y, const Cell& self) = nullptr;
};

const MatProps& matProps(u8 id);
void registerDefaultMaterials();
