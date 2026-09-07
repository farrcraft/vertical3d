/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Engine.h"

#include <string>
#include "../event/Sound.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

namespace v3d::audio {

Engine::Engine(const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<entt::dispatcher> &dispatcher) :
    dispatcher_(dispatcher), logger_(logger) {
}

void Engine::shutdown() {
    // the clips go before the mixer they were played through
    sounds_.clear();
    if (mixer_) {
        MIX_DestroyMixer(mixer_);
        mixer_ = nullptr;
        // the references initialize() took
        MIX_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

bool Engine::initialize() {
    // the mixer opens a device out of SDL's audio subsystem, which the game engine does
    // not ask for - it initialises video only. The subsystem is refcounted, and this call
    // pairs with the SDL_QuitSubSystem in shutdown().
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        logger_->get()->error("could not initialize SDL audio: {}", SDL_GetError());
        return false;
    }
    if (!MIX_Init()) {
        logger_->get()->error("could not initialize the mixer: {}", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    // the default playback device, in whatever format it prefers
    mixer_ = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mixer_) {
        logger_->get()->error("could not open an audio device: {}", SDL_GetError());
        MIX_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    dispatcher_->sink<v3d::event::Sound>().connect<&Engine::soundEvent>(*this);
    return mixer_ != nullptr;
}

void Engine::soundEvent(const v3d::event::Sound& sound) {
    if (!playClip(sound.clip())) {
        logger_->get()->error("unable to play clip: {}", sound.clip());
    }
}

bool Engine::load(const boost::shared_ptr<v3d::asset::Json>& config, const Resolve& resolve) {
    auto const doc = config->document();
    // every lookup is guarded, because boost::json::object::at throws for a key it does
    // not hold and a rejected config has to reach the caller as a false return
    if (!doc.contains("sounds") || !doc.at("sounds").is_array()) {
        logger_->get()->error("Missing sounds in audio config");
        return false;
    }
    auto const items = doc.at("sounds").as_array();
    const auto* it = items.begin();
    bool loaded = true;
    for (; it != items.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("Unrecognized sound config");
            return false;
        }
        auto const sound = it->as_object();
        if (!sound.contains("clip_id") || !sound.contains("file")) {
            logger_->get()->error("Sound config names no clip_id or no file");
            return false;
        }
        const std::string clipId = boost::json::value_to<std::string>(sound.at("clip_id"));
        const std::string fileName = boost::json::value_to<std::string>(sound.at("file"));
        // a clip that will not load leaves the rest of the document to load anyway - one
        // missing wav is not a reason to start an app without any of its sounds
        if (!resolve || !addClip(resolve(fileName), clipId)) {
            logger_->get()->error("unable to load audio clip [{}] from [{}]", clipId, fileName);
            loaded = false;
        }
    }

    return loaded;
}

bool Engine::addClip(const boost::shared_ptr<AudioClip>& clip, const std::string_view& key) {
    // playClip hands the audio to the mixer, so a clip that never read its file is
    // refused here rather than reaching it as a null
    if (!clip || !clip->audio()) {
        return false;
    }
    const std::string clipId(key);
    sounds_[clipId] = clip;
    logger_->get()->debug("SoundEngine::addClip - filed audio clip under id [{}]", clipId);

    return true;
}

bool Engine::playClip(const std::string_view & clip) {
    // an engine whose initialize() opened no device has nothing to play into
    if (!mixer_) {
        return false;
    }
    const std::string clipId(clip);
    auto const found = sounds_.find(clipId);
    if (found == sounds_.end()) {
        return false;
    }
    // fire and forget: the mixer owns the playback, and a clip may overlap itself
    return MIX_PlayAudio(mixer_, found->second->audio());
}

};  // namespace v3d::audio
