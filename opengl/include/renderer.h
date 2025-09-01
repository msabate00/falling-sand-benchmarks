#pragma once
#include <vector>
#include <cstdint>

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Sube el grid (w x h, 1 byte por celda con el índice de material)
    void drawGrid(const std::vector<uint8_t>& indices, int w, int h);

private:
    unsigned int prog = 0, vao = 0, tex = 0;
    int texW = 0, texH = 0;

    void ensureGL();
    void initOnce();
};
