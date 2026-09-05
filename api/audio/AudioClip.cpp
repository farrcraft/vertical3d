/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "AudioClip.h"

#include <string>

namespace v3d::audio {

AudioClip::~AudioClip() {
    destroy();
}

void AudioClip::destroy() {
    if (!audio_) {
        return;
    }
    MIX_DestroyAudio(audio_);
    audio_ = nullptr;
    // the reference this clip took in load()
    MIX_Quit();
}

bool AudioClip::load(const std::string_view & filename) {
    destroy();
    // MIX_Init counts its callers, so a clip loaded with no engine up initialises the
    // library itself and releases it again when it is destroyed
    if (!MIX_Init()) {
        return false;
    }
    const std::string str(filename);
    // no mixer, so the clip outlives any one device and can be read through the asset
    // manager before an app has opened one. Predecoded because these are short effects
    // played from a game tick, where a decode is a stall
    audio_ = MIX_LoadAudio(nullptr, str.c_str(), true);
    if (!audio_) {
        MIX_Quit();
        return false;
    }
    return true;
}

MIX_Audio* AudioClip::audio() const noexcept {
    return audio_;
}

};  // namespace v3d::audio
