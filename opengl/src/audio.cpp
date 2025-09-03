#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"
#include "engine.h"
#include <cmath>

bool Audio::init() {
    if (ready) return true;
    ready = (ma_engine_init(NULL, &eng) == MA_SUCCESS);

    loadAudios();

    return ready;
}

void Audio::loadAudios() {

    load("ignite", AUDIO_DIR  "/ignite.wav", 16);
    load("paint", AUDIO_DIR  "/paint.wav", 1);
}

void Audio::shutdown() {
    for (auto& [_, s] : sfx) {
        for (auto* v : s.voices) {
            if (v) { ma_sound_uninit(v); delete v; }
        }
    }
    sfx.clear();
    if (ready) { ma_engine_uninit(&eng); ready = false; }
}

void Audio::update(Engine& E) {
    std::vector<AudioEvent> evs;
    if (E.takeAudioEvents(evs)) {
        for (const auto& e : evs) {

            float x01 = float(e.x) / float(E.width());
            float y01 = float(e.y) / float(E.height());
            switch (e.type) {
            case AudioEvent::Type::Ignite: play("ignite", x01, y01, 0.9f); break;
            case AudioEvent::Type::Paint:  play("paint", x01, y01, 0.5f); break;
            }
        }
    }
}

bool Audio::load(const std::string& key, const char* path, int voices) {
    if (!ready) return false;
    Sfx s;
    s.voices.reserve(size_t(voices));
    for (int i = 0; i < voices; ++i) {
        auto* v = new ma_sound;
        if (ma_sound_init_from_file(&eng, path,
            MA_SOUND_FLAG_DECODE, NULL, NULL, v) != MA_SUCCESS) {
            delete v;
            return false;
        }
        ma_sound_set_spatialization_enabled(v, MA_FALSE);
        s.voices.push_back(v);
    }
    sfx[key] = std::move(s);
    return true;
}

void Audio::play(const std::string& key, float x01, float y01, float vol) {
    if (!ready) return;
    auto it = sfx.find(key);
    if (it == sfx.end() || it->second.voices.empty()) return;

    x01 = clamp01(x01); y01 = clamp01(y01);
    float pan = x01 * 2.f - 1.f;         
    float att = 1.f - 0.6f * y01;        
    float gain = clamp01(vol * att);

    Sfx& s = it->second;
    ma_sound* v = s.voices[s.cursor];
    s.cursor = (s.cursor + 1) % s.voices.size();

    ma_sound_stop(v);
    ma_sound_seek_to_pcm_frame(v, 0);
    ma_sound_set_pan(v, pan);
    ma_sound_set_volume(v, gain);
    ma_sound_start(v);
}
