#include "renderer.h"
#include "utils.h"
#include <glad/gl.h>
#include <string>
#include <cstring>

static unsigned int makeShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}
static unsigned int makeProgram(const char* vs, const char* fs) {
    unsigned int v = makeShader(GL_VERTEX_SHADER, vs);
    unsigned int f = makeShader(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

Renderer::Renderer() { initOnce(); }
Renderer::~Renderer() {
    if (paletteUBO) glDeleteBuffers(1, &paletteUBO);
    if (pbo[0] || pbo[1]) glDeleteBuffers(2, pbo);

    if (sceneTex) glDeleteTextures(1, &sceneTex);
    if (pingTex[0]) glDeleteTextures(1, &pingTex[0]);
    if (pingTex[1]) glDeleteTextures(1, &pingTex[1]);
    if (sceneFBO) glDeleteFramebuffers(1, &sceneFBO);
    if (pingFBO[0]) glDeleteFramebuffers(1, &pingFBO[0]);
    if (pingFBO[1]) glDeleteFramebuffers(1, &pingFBO[1]);

    if (tex) glDeleteTextures(1, &tex);
    if (vao) glDeleteVertexArrays(1, &vao);

    if (progComposite) glDeleteProgram(progComposite);
    if (progBlur) glDeleteProgram(progBlur);
    if (progThresh) glDeleteProgram(progThresh);
    if (progGrid) glDeleteProgram(progGrid);
}
void Renderer::ensureGL() { if (!progGrid) initOnce(); }

void Renderer::initOnce() {
    // --- Grid program (índices + paleta UBO + discos) ---
    std::string vsSrc = readTextFile(SHADER_DIR "/grid.vs.glsl");
    std::string fsSrc = readTextFile(SHADER_DIR "/grid.fs.glsl");
    progGrid = makeProgram(vsSrc.c_str(), fsSrc.c_str());

    glGenVertexArrays(1, &vao);
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    loc_uTex = glGetUniformLocation(progGrid, "uTex");
    loc_uGrid = glGetUniformLocation(progGrid, "uGrid");
    loc_uView = glGetUniformLocation(progGrid, "uView");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Paleta UBO (RGBA 0..1)
    glGenBuffers(1, &paletteUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, paletteUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4 * 256*2, nullptr, GL_STATIC_DRAW);

    std::vector<float> colors(4 * 256, 0.0f);
    std::vector<float> extra(4 * 256, 0.0f);
    for (int i = 0; i < 256; ++i) {
        const MatProps& mp = matProps((u8)i);
        colors[4 * i + 0] = mp.r / 255.0f;
        colors[4 * i + 1] = mp.g / 255.0f;
        colors[4 * i + 2] = mp.b / 255.0f;
        colors[4 * i + 3] = mp.a / 255.0f;
        extra[4 * i + 0] = mp.emissive;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, colors.size() * sizeof(float), colors.data());
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(float) * 4 * 256, extra.size() * sizeof(float), extra.data());

    GLuint blk = glGetUniformBlockIndex(progGrid, "Palette");
    glUniformBlockBinding(progGrid, blk, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, paletteUBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Post programs ---
    std::string vsPost = vsSrc;
    std::string fsThresh = readTextFile(SHADER_DIR "/post_threshold.fs.glsl");
    std::string fsBlur = readTextFile(SHADER_DIR "/post_blur.fs.glsl");
    std::string fsComp = readTextFile(SHADER_DIR "/post_composite.fs.glsl");

    progThresh = makeProgram(vsPost.c_str(), fsThresh.c_str());
    progBlur = makeProgram(vsPost.c_str(), fsBlur.c_str());
    progComposite = makeProgram(vsPost.c_str(), fsComp.c_str());

    loc_th_uScene = glGetUniformLocation(progThresh, "uScene");
    loc_th_uThreshold = glGetUniformLocation(progThresh, "uThreshold");

    loc_bl_uTex = glGetUniformLocation(progBlur, "uTex");
    loc_bl_uTexel = glGetUniformLocation(progBlur, "uTexel");
    loc_bl_uHorizontal = glGetUniformLocation(progBlur, "uHorizontal");

    loc_cp_uScene = glGetUniformLocation(progComposite, "uScene");
    loc_cp_uBloom = glGetUniformLocation(progComposite, "uBloom");
    loc_cp_uExposure = glGetUniformLocation(progComposite, "uExposure");
    loc_cp_uBloomStrength = glGetUniformLocation(progComposite, "uBloomStrength");
}

void Renderer::ensureSceneTargets(int viewW, int viewH) {
    if (viewW <= 0 || viewH <= 0) return;
    if (fboW == viewW && fboH == viewH && sceneFBO) return;

    // Destroy previous
    if (sceneTex) { glDeleteTextures(1, &sceneTex); sceneTex = 0; }
    if (pingTex[0]) { glDeleteTextures(1, &pingTex[0]); pingTex[0] = 0; }
    if (pingTex[1]) { glDeleteTextures(1, &pingTex[1]); pingTex[1] = 0; }
    if (sceneFBO) { glDeleteFramebuffers(1, &sceneFBO); sceneFBO = 0; }
    if (pingFBO[0]) { glDeleteFramebuffers(1, &pingFBO[0]); pingFBO[0] = 0; }
    if (pingFBO[1]) { glDeleteFramebuffers(1, &pingFBO[1]); pingFBO[1] = 0; }

    fboW = viewW; fboH = viewH;

    auto makeColorTex = [&](unsigned int& t) {
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, fboW, fboH, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        };

    glGenFramebuffers(1, &sceneFBO);
    makeColorTex(sceneTex);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

    for (int i = 0;i < 2;++i) {
        glGenFramebuffers(1, &pingFBO[i]);
        makeColorTex(pingTex[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingTex[i], 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::uploadFullCPU(const std::uint8_t* img, int w, int h) {
    if (!img || w <= 0 || h <= 0) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    if (w != texW || h != texH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = w; texH = h;
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, img);
    texValid = true;
}

void Renderer::uploadRectPBO(const std::uint8_t* src, int rw, int rh, int x0, int y0, int texWNeeded, int texHNeeded) {
    if (rw <= 0 || rh <= 0) return;

    glBindTexture(GL_TEXTURE_2D, tex);
    if (texW != texWNeeded || texH != texHNeeded) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, texWNeeded, texHNeeded, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = texWNeeded; texH = texHNeeded;
    }

    const size_t bytes = size_t(rw) * size_t(rh);
    if (pbo[0] == 0 && pbo[1] == 0) glGenBuffers(2, pbo);
    if (pboCapacity < bytes) {
        for (int i = 0;i < 2;++i) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, bytes, nullptr, GL_STREAM_DRAW);
        }
        pboCapacity = bytes;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[pboIdx]);
    void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, bytes,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    std::memcpy(ptr, src, bytes);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, rw);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, rw, rh, GL_RED_INTEGER, GL_UNSIGNED_BYTE, (const void*)0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    pboIdx ^= 1;
}

void Renderer::drawFullscreen() {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

// ------------------------------- DRAW -------------------------------
void Renderer::drawGrid(const std::vector<uint8_t>& indices, int w, int h, int viewW, int viewH) {
    ensureGL();
    if (!indices.empty()) {
        uploadFullCPU(indices.data(), w, h);
    }

    ensureSceneTargets(viewW, viewH);

    // 1) Grid → HDR scene
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, viewW, viewH);
    glClearColor(0.00f, 0.00f, 0.00f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(progGrid);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(loc_uTex, 0);
    glUniform2f(loc_uGrid, float(w), float(h));
    glUniform2f(loc_uView, float(viewW), float(viewH));
    drawFullscreen();

    // 2) Bloom: threshold → blur ping-pong
    glDisable(GL_BLEND);

    glUseProgram(progThresh);
    glUniform1i(loc_th_uScene, 0);
    glUniform1f(loc_th_uThreshold, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[0]);
    glViewport(0, 0, viewW, viewH);
    drawFullscreen();

    bool horizontal = true;
    int passes = 6;
    for (int i = 0;i < passes;++i) {
        glUseProgram(progBlur);
        glUniform1i(loc_bl_uTex, 0);
        glUniform2f(loc_bl_uTexel, 1.0f / float(viewW), 1.0f / float(viewH));
        glUniform1i(loc_bl_uHorizontal, horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pingTex[horizontal ? 0 : 1]);

        glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[horizontal ? 1 : 0]);
        drawFullscreen();
        horizontal = !horizontal;
    }

    // 3) Composite → backbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewW, viewH);
    glUseProgram(progComposite);
    glUniform1i(loc_cp_uScene, 0);
    glUniform1i(loc_cp_uBloom, 1);
    glUniform1f(loc_cp_uExposure, 1.0f);
    glUniform1f(loc_cp_uBloomStrength, 0.7f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingTex[horizontal ? 0 : 1]); // último blur

    drawFullscreen();
    glEnable(GL_BLEND);
}

void Renderer::draw(const std::vector<Cell>& cells, int w, int h, int viewW, int viewH) {
    // Fallback: pack completo (por compatibilidad)
    scratch.resize(size_t(w) * size_t(h));
    for (int y = 0; y < h; ++y) {
        const Cell* row = &cells[size_t(y) * size_t(w)];
        uint8_t* dst = &scratch[size_t(y) * size_t(w)];
        for (int x = 0; x < w; ++x) dst[x] = row[x].m;
    }
    drawGrid(scratch, w, h, viewW, viewH);
}

void Renderer::drawPlane(const std::uint8_t* planeM, int w, int h,
    int viewW, int viewH, int x0, int y0, int rw, int rh) {
    ensureGL();

    if (!texValid || texW != w || texH != h) {
        // primera vez o resize → full upload
        uploadFullCPU(planeM, w, h);
    }
    else if (rw > 0 && rh > 0) {
        // empaquetar SOLO el rect sucio contiguo y subir vía PBO
        scratchRect.resize(size_t(rw) * size_t(rh));
        for (int y = 0; y < rh; ++y) {
            const uint8_t* srcRow = &planeM[size_t(y0 + y) * size_t(w) + size_t(x0)];
            uint8_t* dstRow = &scratchRect[size_t(y) * size_t(rw)];
            std::memcpy(dstRow, srcRow, size_t(rw));
        }
        uploadRectPBO(scratchRect.data(), rw, rh, x0, y0, w, h);
    }

    drawGrid(std::vector<uint8_t>{}, w, h, viewW, viewH); // usa texturas ya subidas + post
}
