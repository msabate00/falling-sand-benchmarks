#pragma once
#include <cstdint>

enum class Material : uint8_t { Empty = 0, Sand, Water, Stone, Wood, Fire, Smoke };

struct Cell {
    Material m = Material::Empty;
    uint8_t  meta = 0;
};
