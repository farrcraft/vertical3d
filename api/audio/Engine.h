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

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "AudioClip.h"
#include "../asset/Json.h"
#include "../log/Logger.h"
#include "../event/Sound.h"

#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

namespace v3d::audio {

/**
 * A sound that has been started, for as long as it is playing.
 *
 * An id rather than a pointer, so that a handle to a sound that has since finished is
 * refused rather than being a pointer to a track the engine has recycled underneath it.
 * Zero is no sound at all, which is what a failed play() gives back.
 **/
typedef uint32_t Voice;

/**
 * How to start a sound. Every field has the value a one shot wants, so the default is what
 * playClip() has always done.
 **/
struct Play final {
    Play() noexcept;

    /**
     * The bus the sound is mixed on - "music", "sfx", "ambience". A tag is a named group
     * with a volume for the cost of a string, which is what makes a settings screen three
     * sliders rather than one. Empty is no bus.
     **/
    std::string bus;

    /**
     * How many times to repeat after the first play. -1 loops until stopped, which is what
     * a bed of ambience or a music track wants.
     **/
    int loops;

    /**
     * How long to fade up from silence, in milliseconds, so a bed starts without a click.
     **/
    int fadeInMs;

    /**
     * The sound's own volume, multiplied by its bus's. 1 is unchanged.
     **/
    float gain;
};

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

    /**
     * Start a clip and forget it, which is what a one shot is.
     *
     * @return whether it started
     **/
    bool playClip(const std::string_view & clip);

    /**
     * Start a clip and keep hold of it.
     *
     * @param clip the id the clip was filed under
     * @param how the bus, the looping, the fade and the gain
     * @return the voice, or 0 when there is no device, no such clip, or no track to be had
     **/
    Voice play(const std::string_view & clip, const Play & how);

    /**
     * Stop one sound.
     *
     * @param voice what play() gave back
     * @param fadeOutMs how long to fade down to silence first, so a bed ends without a click
     * @return false for a voice that is not playing, which a finished one shot is
     **/
    bool stop(Voice voice, int fadeOutMs = 0);

    /**
     * Stop everything, on every bus.
     **/
    void stopAll(int fadeOutMs = 0);

    /**
     * @return whether the sound is still going
     **/
    bool playing(Voice voice) const;

    /**
     * Set one sound's own volume, which multiplies its bus's.
     *
     * @return false for a voice that is not playing
     **/
    bool gain(Voice voice, float level);

    /**
     * Set a bus's volume, which every sound started on it is multiplied by.
     *
     * The bus need not exist yet: a settings screen writes the volumes before anything has
     * played, and a sound started on that bus afterwards is mixed at what was set.
     *
     * @return false when there is no device
     **/
    bool busGain(const std::string & bus, float level);

    void soundEvent(const v3d::event::Sound & sound);

 private:
    /**
     * A track the mixer is playing, and what it was started as.
     **/
    struct Playing final {
        MIX_Track* track;
        std::string bus;
    };

    /**
     * Take the tracks of every finished sound back, so a game that starts one shots does
     * not grow a track per sound played.
     *
     * Called as a sound is started rather than on a timer, because that is the only moment
     * the engine is asked for anything and a finished track costs nothing until then.
     **/
    void reap();

    /**
     * @return a track off the free list, or a new one, or null when the mixer will give
     *         out no more
     **/
    MIX_Track* claim();

    boost::shared_ptr<entt::dispatcher> dispatcher_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    MIX_Mixer* mixer_ = nullptr;
    std::map<std::string, boost::shared_ptr<AudioClip>> sounds_;
    std::map<Voice, Playing> voices_;
    std::vector<MIX_Track*> free_;   /**< reaped tracks, waiting to be played again **/
    Voice nextVoice_ = 1;            /**< never reused, so a stale handle stays stale **/
};

};  // namespace v3d::audio
