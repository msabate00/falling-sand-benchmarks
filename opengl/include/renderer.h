#pragma once
#include <vector>
#include <cstdint>
#include "material.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Fallback (sube todo desde Cells)
    void draw(const std::vector<Cell>& cells, int w, int h, int viewW, int viewH);

    // Ruta óptima: plano SoA + dirty-rect (si rw/rh==0 no sube nada)
    void drawPlane(const std::uint8_t* planeM, int w, int h,
        int viewW, int viewH, int x0, int y0, int rw, int rh);

    // Útil si quieres subir todo el plano SoA directamente
    void drawGrid(const std::vector<uint8_t>& indices, int w, int h, int viewW, int viewH);

private:
    // --- Grid pass (índices → color con paleta UBO + discos) ---
    unsigned int progGrid = 0, vao = 0, tex = 0;
    int texW = 0, texH = 0;
    bool texValid = false;

    unsigned int paletteUBO = 0;

    int loc_uTex = -1;
    int loc_uGrid = -1;
    int loc_uView = -1;

    // --- PBO doble para uploads ---
    unsigned int pbo[2] = { 0,0 };
    int pboIdx = 0;
    size_t pboCapacity = 0; // bytes

    // --- Post: HDR + Bloom ---
    unsigned int progThresh = 0, progBlur = 0, progComposite = 0;
    int loc_th_uScene = -1, loc_th_uThreshold = -1;
    int loc_bl_uTex = -1, loc_bl_uTexel = -1, loc_bl_uHorizontal = -1;
    int loc_cp_uScene = -1, loc_cp_uBloom = -1, loc_cp_uExposure = -1, loc_cp_uBloomStrength = -1;

    unsigned int sceneFBO = 0, sceneTex = 0;
    unsigned int pingFBO[2] = { 0,0 }, pingTex[2] = { 0,0 };
    int fboW = 0, fboH = 0;

    // CPU buffers
    std::vector<uint8_t> scratch;     // full
    std::vector<uint8_t> scratchRect; // rect

    void ensureGL();
    void initOnce();
    void ensureSceneTargets(int viewW, int viewH);

    void uploadFullCPU(const std::uint8_t* img, int w, int h);
    void uploadRectPBO(const std::uint8_t* src, int rw, int rh, int x0, int y0,
        int texWNeeded, int texHNeeded);

    // draws a full-screen triangle with the currently bound program & textures
    void drawFullscreen();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
};
