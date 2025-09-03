#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "./../third_party/miniaudio/miniaudio.h"


struct Engine;

class Audio {
public:
    bool init();
    void shutdown();

    
    void loadAudios();

    void play(const std::string& key, float x01, float y01, float vol = 1.0f);

    void update(Engine& E);

private:


    bool load(const std::string& key, const char* path, int voices = 8);

    ma_engine eng{};
    bool ready = false;

    struct Sfx {
        std::vector<ma_sound*> voices;
        size_t cursor = 0;
    };
    std::unordered_map<std::string, Sfx> sfx;

    static float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
};
