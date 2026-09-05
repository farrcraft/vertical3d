/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <SDL3_mixer/SDL_mixer.h>

#include <string>

namespace v3d::audio {

/**
 * One loaded sound.
 *
 * A clip is loaded against no mixer, so it can be read through the asset manager without
 * an audio::Engine and played through whichever mixer an app later opens. It holds a
 * MIX_Init reference for as long as it holds audio, because the library counts them.
 **/
class AudioClip final {
 public:
    AudioClip() = default;
    ~AudioClip();

    // the clip owns its MIX_Audio and the MIX_Init reference that came with it
    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;

    bool load(const std::string_view & filename);
    void destroy();

    /**
     * @return what was loaded, or null for a clip that holds nothing
     **/
    MIX_Audio* audio() const noexcept;

 private:
    MIX_Audio* audio_ = nullptr;
};

};  // namespace v3d::audio
