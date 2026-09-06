/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

// Fix necessary for EnTT - https://github.com/skypjack/entt/issues/96#issuecomment-395867237
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <SDL3_mixer/SDL_mixer.h>

#include <functional>
#include <map>
#include <string>

#include "AudioClip.h"
#include "../asset/Json.h"
#include "../log/Logger.h"
#include "../event/Sound.h"

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::audio {

/**
 * An Audio / Sound processing engine
 **/
class Engine final {
 public:
    Engine(const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<entt::dispatcher>& dispatcher);
    ~Engine() = default;

    /**
     * What turns the source a sound config names into a loaded clip, per ADR-0021.
     *
     * This library never reaches the asset manager: `v3dlib_asset` loads through
     * `v3dlib_audio`, so the dependency cannot run both ways.
     **/
    typedef std::function<boost::shared_ptr<AudioClip>(const std::string& source)> Resolve;

    /**
     * Open an audio device.
     *
     * @return false when none opened, which leaves the engine silent rather than unusable -
     *         a clip still loads and files, and playing one is a false return
     **/
    bool initialize();
    void shutdown();

    /**
     * Load every clip a sound config names, through the resolver the app supplies.
     *
     * @param config the document, an array of clip ids over the files that hold them
     * @param resolve what turns one of those files into a clip
     * @return false when the document is malformed, or when a clip it named would not
     *         load - the clips that did load are kept either way
     **/
    bool load(const boost::shared_ptr<v3d::asset::Json> & config, const Resolve & resolve);

    /**
     * File an already loaded clip under the id a sound event will name.
     *
     * @return false for a clip holding no audio, which playClip would hand to the mixer
     *         as a null
     **/
    bool addClip(const boost::shared_ptr<AudioClip> & clip, const std::string_view & key);
    bool playClip(const std::string_view & clip);
    void soundEvent(const v3d::event::Sound & sound);

 private:
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    MIX_Mixer* mixer_ = nullptr;
    std::map<std::string, boost::shared_ptr<AudioClip>> sounds_;
};

};  // namespace v3d::audio
