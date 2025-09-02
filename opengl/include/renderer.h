#pragma once
#include <vector>
#include <cstdint>
#include "material.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    void draw(const std::vector<Cell>& cells, int w, int h, int viewW, int viewH);
    void draw(const std::vector<Cell>& cells, int w, int h,
        int viewW, int viewH, int x0, int y0, int rw, int rh);

    void drawGrid(const std::vector<uint8_t>& indices, int w, int h, int viewW, int viewH);

private:
    unsigned int prog = 0, vao = 0, tex = 0;
    int texW = 0, texH = 0;

    unsigned int paletteUBO = 0;

    unsigned int pbo[2] = { 0, 0 };
    int pboIdx = 0;
    size_t pboCapacity = 0; // bytes reservados en PBO

    int loc_uTex = -1;
    int loc_uGrid = -1;
    int loc_uView = -1;

    std::vector<uint8_t> scratch;     // para full upload
    std::vector<uint8_t> scratchRect; // solo el rect sucio

    void ensureGL();
    void initOnce();

    void uploadFullCPU(const std::vector<uint8_t>& img, int w, int h);
    void uploadRectPBO(const uint8_t* src, int rw, int rh, int x0, int y0, int texWNeeded, int texHNeeded);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};
