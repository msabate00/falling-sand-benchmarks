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
    if (tex) glDeleteTextures(1, &tex);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (prog) glDeleteProgram(prog);
}
void Renderer::ensureGL() { if (!prog) initOnce(); }

void Renderer::initOnce() {
    std::string vsSrc = readTextFile(SHADER_DIR "/grid.vs.glsl");
    std::string fsSrc = readTextFile(SHADER_DIR "/grid.fs.glsl");
    prog = makeProgram(vsSrc.c_str(), fsSrc.c_str());

    glGenVertexArrays(1, &vao);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    loc_uTex = glGetUniformLocation(prog, "uTex");
    loc_uGrid = glGetUniformLocation(prog, "uGrid");
    loc_uView = glGetUniformLocation(prog, "uView");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenBuffers(1, &paletteUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, paletteUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4 * 256, nullptr, GL_STATIC_DRAW);

    std::vector<float> pal(4 * 256, 0.0f);
    for (int i = 0; i < 256; ++i) {
        const MatProps& mp = matProps((u8)i);
        pal[4 * i + 0] = mp.r / 255.0f;
        pal[4 * i + 1] = mp.g / 255.0f;
        pal[4 * i + 2] = mp.b / 255.0f;
        pal[4 * i + 3] = mp.a / 255.0f;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, pal.size() * sizeof(float), pal.data());
    GLuint blk = glGetUniformBlockIndex(prog, "Palette");
    glUniformBlockBinding(prog, blk, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, paletteUBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::uploadFullCPU(const std::vector<uint8_t>& img, int w, int h) {
    glBindTexture(GL_TEXTURE_2D, tex);
    if (w != texW || h != texH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = w; texH = h;
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, img.data());
}

void Renderer::uploadRectPBO(const uint8_t* src, int rw, int rh, int x0, int y0, int texWNeeded, int texHNeeded) {
    if (rw <= 0 || rh <= 0) return;

    glBindTexture(GL_TEXTURE_2D, tex);
    if (texW != texWNeeded || texH != texHNeeded) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, texWNeeded, texHNeeded, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        texW = texWNeeded; texH = texHNeeded;
    }

    const size_t bytes = size_t(rw) * size_t(rh);
    if (pbo[0] == 0 && pbo[1] == 0) glGenBuffers(2, pbo);
    if (pboCapacity < bytes) {
        for (int i = 0; i < 2; ++i) {
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
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, rw, rh, GL_RED_INTEGER, GL_UNSIGNED_BYTE, (const void*)0);


    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    pboIdx ^= 1;
}

void Renderer::drawGrid(const std::vector<uint8_t>& indices, int w, int h, int viewW, int viewH) {
    ensureGL();

    uploadFullCPU(indices, w, h);

    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(loc_uTex, 0);
    glUniform2f(loc_uGrid, float(w), float(h));
    glUniform2f(loc_uView, float(viewW), float(viewH));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::draw(const std::vector<Cell>& cells, int w, int h, int viewW, int viewH) {
    scratch.resize(size_t(w) * size_t(h));
    for (int y = 0; y < h; ++y) {
        const Cell* row = &cells[size_t(y) * size_t(w)];
        uint8_t* dst = &scratch[size_t(y) * size_t(w)];
        for (int x = 0; x < w; ++x) dst[x] = row[x].m;
    }
    drawGrid(scratch, w, h, viewW, viewH);
}

void Renderer::draw(const std::vector<Cell>& cells, int w, int h,
    int viewW, int viewH, int x0, int y0, int rw, int rh) {
    ensureGL();

    if (rw > 0 && rh > 0) {
        
        scratchRect.resize(size_t(rw) * size_t(rh));
        for (int y = 0; y < rh; ++y) {
            const Cell* srcRow = &cells[size_t(y0 + y) * size_t(w) + size_t(x0)];
            uint8_t* dstRow = &scratchRect[size_t(y) * size_t(rw)];
            for (int x = 0; x < rw; ++x) dstRow[x] = srcRow[x].m;
        }
        
        uploadRectPBO(scratchRect.data(), rw, rh, x0, y0, w, h);
    }

    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(loc_uTex, 0);
    glUniform2f(loc_uGrid, float(w), float(h));
    glUniform2f(loc_uView, float(viewW), float(viewH));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
