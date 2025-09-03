#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "./../third_party/miniaudio/miniaudio.h"

class Audio {
public:
    bool init();
    void shutdown();

    
    bool load(const std::string& key, const char* path, int voices = 8);

    void play(const std::string& key, float x01, float y01, float vol = 1.0f);

private:
    ma_engine eng{};
    bool ready = false;

    struct Sfx {
        std::vector<ma_sound*> voices;
        size_t cursor = 0;
    };
    std::unordered_map<std::string, Sfx> sfx;

    static float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
};
