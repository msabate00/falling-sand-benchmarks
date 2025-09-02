#pragma once
#include <vector>
#include <cstdint>
#include "material.h"


class Renderer {
public:
    Renderer();
    ~Renderer();

    void draw(const std::vector<Cell>& cells, int w, int h, bool points, float feather = 1.0f);
    void drawGrid(const std::vector<uint8_t>& indices, int w, int h);


private:
    unsigned int prog = 0, vao = 0, tex = 0;
    int texW = 0, texH = 0;

    unsigned int pointsProg = 0, pointsVAO = 0, pointsVBO = 0, paletteUBO = 0;

    std::vector<uint8_t> scratch;

    void ensureGL();
    void initOnce();

    void initPointsOnce();
    void drawPoints(const std::vector<Cell>& cells, int w, int h, float feather);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};
