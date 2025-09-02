#pragma once
#include <vector>
#include <cstdint>
#include "material.h"

class Renderer {
public:
    Renderer();
    ~Renderer();


    void draw(const std::vector<Cell>& cells, int w, int h, int viewW, int viewH);

    void drawGrid(const std::vector<uint8_t>& indices, int w, int h, int viewW, int viewH);

private:
    unsigned int prog = 0, vao = 0, tex = 0;
    int texW = 0, texH = 0;

    unsigned int paletteUBO = 0;

    int loc_uTex = -1;
    int loc_uGrid = -1;
    int loc_uView = -1;

    std::vector<uint8_t> scratch;

    void ensureGL();
    void initOnce();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};
