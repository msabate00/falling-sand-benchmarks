#pragma once
#include <cstdint>

enum class Material : uint8_t { Empty = 0, Sand = 1, Water = 2, Stone = 3 };

struct Cell {
    Material m = Material::Empty;
    uint8_t  meta = 0;
};
