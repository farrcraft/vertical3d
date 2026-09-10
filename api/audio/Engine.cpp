/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Engine.h"

#include <api/event/kind/Sound.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

namespace v3d::audio {

/**
 **/
Play::Play() noexcept :
loops(0),
fadeInMs(0),
gain(1.0f) {
}

Engine::Engine(const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<entt::dispatcher> &dispatcher) :
    dispatcher_(dispatcher), logger_(logger) {
}

void Engine::shutdown() {
    // the tracks go before the mixer that handed them out, and the clips before the tracks
    // that were playing them
    for (const std::pair<const Voice, Playing>& playing : voices_) {
        MIX_DestroyTrack(playing.second.track);
    }
    voices_.clear();
    for (MIX_Track* track : free_) {
        MIX_DestroyTrack(track);
    }
    free_.clear();
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

    dispatcher_->sink<v3d::event::kind::Sound>().connect<&Engine::soundEvent>(*this);
    return mixer_ != nullptr;
}

void Engine::soundEvent(const v3d::event::kind::Sound& sound) {
    if (!playClip(sound.clip())) {
        logger_->get()->error("unable to play clip: {}", sound.clip());
    }
}

bool Engine::load(const boost::shared_ptr<v3d::asset::kind::Json>& config, const Resolve& resolve) {
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
    // a one shot nobody holds is still a track underneath, so that it can be stopped by
    // stopAll() and mixed on whatever the master gain is
    return play(clip, Play()) != 0;
}

void Engine::reap() {
    for (std::map<Voice, Playing>::iterator playing = voices_.begin(); playing != voices_.end();) {
        if (MIX_TrackPlaying(playing->second.track)) {
            ++playing;
            continue;
        }
        if (!playing->second.bus.empty()) {
            // an untagged track, so that being played again on another bus does not leave
            // it mixed on both
            MIX_UntagTrack(playing->second.track, playing->second.bus.c_str());
        }
        free_.push_back(playing->second.track);
        playing = voices_.erase(playing);
    }
}

MIX_Track* Engine::claim() {
    if (!free_.empty()) {
        MIX_Track* track = free_.back();
        free_.pop_back();
        return track;
    }
    return MIX_CreateTrack(mixer_);
}

Voice Engine::play(const std::string_view & clip, const Play & how) {
    // an engine whose initialize() opened no device has nothing to play into
    if (!mixer_) {
        return 0;
    }
    const std::string clipId(clip);
    const std::map<std::string, boost::shared_ptr<AudioClip>>::const_iterator found = sounds_.find(clipId);
    if (found == sounds_.end()) {
        return 0;
    }

    reap();
    MIX_Track* track = claim();
    if (track == nullptr) {
        logger_->get()->error("no track to play clip [{}] on: {}", clipId, SDL_GetError());
        return 0;
    }
    if (!MIX_SetTrackAudio(track, found->second->audio())) {
        logger_->get()->error("unable to put clip [{}] on a track: {}", clipId, SDL_GetError());
        free_.push_back(track);
        return 0;
    }

    if (!how.bus.empty()) {
        MIX_TagTrack(track, how.bus.c_str());
    }
    MIX_SetTrackGain(track, how.gain);

    // the properties are the only way to name a loop count or a fade, and the id belongs to
    // this call rather than to the track
    const SDL_PropertiesID options = SDL_CreateProperties();
    if (options != 0) {
        if (how.loops != 0) {
            SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, how.loops);
        }
        if (how.fadeInMs > 0) {
            SDL_SetNumberProperty(options, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, how.fadeInMs);
        }
    }
    const bool started = MIX_PlayTrack(track, options);
    if (options != 0) {
        SDL_DestroyProperties(options);
    }
    if (!started) {
        logger_->get()->error("unable to play clip [{}]: {}", clipId, SDL_GetError());
        if (!how.bus.empty()) {
            MIX_UntagTrack(track, how.bus.c_str());
        }
        free_.push_back(track);
        return 0;
    }

    const Voice voice = nextVoice_++;
    Playing playing;
    playing.track = track;
    playing.bus = how.bus;
    voices_[voice] = playing;
    return voice;
}

bool Engine::stop(Voice voice, int fadeOutMs) {
    const std::map<Voice, Playing>::const_iterator found = voices_.find(voice);
    if (found == voices_.end()) {
        return false;
    }
    // MIX_StopTrack counts a fade in sample frames, which is the track's own rate
    const Sint64 frames = fadeOutMs > 0 ? MIX_TrackMSToFrames(found->second.track, fadeOutMs) : 0;
    return MIX_StopTrack(found->second.track, frames > 0 ? frames : 0);
}

void Engine::stopAll(int fadeOutMs) {
    if (!mixer_) {
        return;
    }
    MIX_StopAllTracks(mixer_, fadeOutMs > 0 ? fadeOutMs : 0);
    // not reaped here: a fade is still playing, and a track taken back mid fade would be
    // handed to the next sound before it had finished
}

bool Engine::playing(Voice voice) const {
    const std::map<Voice, Playing>::const_iterator found = voices_.find(voice);
    return found != voices_.end() && MIX_TrackPlaying(found->second.track);
}

bool Engine::gain(Voice voice, float level) {
    const std::map<Voice, Playing>::const_iterator found = voices_.find(voice);
    if (found == voices_.end()) {
        return false;
    }
    return MIX_SetTrackGain(found->second.track, level);
}

bool Engine::busGain(const std::string & bus, float level) {
    if (!mixer_ || bus.empty()) {
        return false;
    }
    return MIX_SetTagGain(mixer_, bus.c_str(), level);
}

};  // namespace v3d::audio
