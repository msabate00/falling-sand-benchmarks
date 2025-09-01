#pragma once

enum class Material : uint8_t { Empty, Sand, Water, Stone };

struct Cell {
    Material m;
    uint8_t meta = 0;
};
