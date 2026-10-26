/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

// Fix necessary for EnTT - https://github.com/skypjack/entt/issues/96#issuecomment-395867237
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <api/asset/Manager.h>
#include <api/event/kind/Sound.h>
#include <api/log/Logger.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "AudioClip.h"
#include "Events.h"
#include "Listener.h"
#include "Play.h"

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>

#include <entt/entt.hpp>

#include <glm/vec3.hpp>

namespace v3d::audio {

/**
 * A sound that has been started, for as long as it is playing.
 *
 * An id, not a pointer. The engine reuses the track of a finished sound, and a handle to that
 * sound is refused instead of reaching the reused track. None is no sound at all, and a failed
 * play() returns it. Every other value comes from a counter in play(), so the base type is as
 * wide as that counter.
 **/
// NOLINTNEXTLINE(performance-enum-size)
enum class Voice : uint32_t { None = 0 };

/**
 * Plays loaded clips through one audio device, and plays each Sound event the dispatcher
 * carries.
 *
 * A sound is either panned between the speakers or placed in the world. A placed sound is heard
 * from the listener, which the game moves once a frame in its world coordinates. The engine
 * gives the mixer each placed sound's position relative to the listener.
 **/
class Player final {
 public:
    /**
     * The name config.json gives the kind of a sound document.
     **/
    static constexpr std::string_view DOCUMENT = "sound";

    Player(const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<entt::dispatcher>& dispatcher);
    /**
     * Calls shutdown(), so the device, the tracks and the dispatcher connection are released
     * whether or not the owner called shutdown() itself.
     **/
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    /**
     * Turns the source a sound config names into a loaded clip. An app whose clips do not
     * come from files supplies one of these. The asset::Manager overload of load() wraps the
     * manager in one, for an app that loads its clips as assets.
     **/
    typedef std::function<boost::shared_ptr<AudioClip>(const std::string& source)> Resolve;

    /**
     * Open an audio device.
     *
     * @return false when no device opened. The engine is then silent but still usable: a
     *         clip still loads and is stored, and play() returns Voice::None
     **/
    bool initialize();
    void shutdown();

    /**
     * Load every clip a sound config names, through the resolver the app supplies.
     *
     * @param doc the document, whose "sounds" array holds a clip_id and a file for each clip
     * @param resolve what turns one of those files into a clip
     * @return false when the document is malformed, or when a clip it named would not
     *         load. The clips that did load are kept either way
     **/
    bool load(const boost::json::object & doc, const Resolve & resolve);

    /**
     * Load every clip a sound config names through an asset manager, which must have had
     * registerLoaders() called on it. This saves every app that plays sound writing the same
     * resolver. A source resolves against the manager's path like any other asset.
     **/
    bool load(const boost::json::object & config, v3d::asset::Manager & assets);

    /**
     * Store an already loaded clip under the id a sound event will name.
     *
     * @return false for a clip holding no audio, which play() would otherwise hand to the
     *         mixer as a null
     **/
    bool addClip(const boost::shared_ptr<AudioClip> & clip, const std::string_view & key);

    /**
     * Whether a clip is stored under an id. This needs no device, so it shows what load()
     * and addClip() stored on a machine that has none.
     **/
    bool has(const std::string_view & clip) const;

    /**
     * Start a clip without keeping a handle to it: a one shot.
     *
     * @return whether it started
     **/
    bool playClip(const std::string_view & clip);

    /**
     * Start a clip and keep hold of it.
     *
     * @param clip the id the clip was filed under
     * @param how the bus, the looping, the fade and the gain
     * @return the voice, or Voice::None when there is no device, no such clip, or no track to
     *         be had
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
     * Set one sound's own volume. The sound still plays at this times its bus's level, so
     * this does not undo a busGain().
     *
     * @return false for a voice that is not playing
     **/
    bool gain(Voice voice, float level);

    /**
     * @return the volume a sound plays at, which is its own gain times its bus's level, as
     *         the mixer holds it. 0 for a voice that is not playing
     **/
    float volume(Voice voice) const;

    /**
     * Move one sound between the speakers, as Play::pan places it when it starts. This clears
     * the position of a placed sound.
     *
     * @return false for a voice that is not playing
     **/
    bool pan(Voice voice, float value);

    /**
     * Move one sound to a point in the world, as Play::position places it when it starts. This
     * clears the pan of a panned sound.
     *
     * @return false for a voice that is not playing
     **/
    bool position(Voice voice, const glm::vec3& position);

    /**
     * @return where the mixer hears a placed sound from, relative to the listener: x to the
     *         right, y up and z behind. Nothing for a sound that is not placed, and for a voice
     *         that is not playing
     **/
    std::optional<glm::vec3> heard(Voice voice) const;

    /**
     * Set where sound is heard from, in the world coordinates a sound's position is in. Every
     * placed sound is heard from here at once, and so is every event of the backend attach()
     * was given. A game calls this once a frame, from its camera or its player.
     *
     * The listener is kept with or without a device, and starts at the origin facing -z with
     * +x to its right.
     *
     * @return false for a listener that faces no way: a forward, an up or a right of zero, a
     *         forward and an up that are parallel, a right in their plane, or a value that is
     *         not finite. The last listener is kept
     **/
    bool listener(const Listener& listener);

    /**
     * @return the listener, as orthonormal() made it
     **/
    const Listener& listener() const noexcept;

    /**
     * Hear events from the same listener as clips. The backend is given the listener now, and
     * again every time listener() is set. Null detaches it.
     **/
    void attach(const boost::shared_ptr<Events>& events);

    /**
     * Set the level of a bus. Every sound on the bus plays at its own gain times this level:
     * the sounds playing on it now, and every sound started on it later.
     *
     * The level is kept until it is set again, so it can be set before initialize(), before
     * anything plays, and with no device at all. A bus never set is at 1.
     *
     * @return false for an empty bus name, which is no bus and has no level, or when a playing
     *         sound's volume could not be changed. The level is stored in that second case
     **/
    bool busGain(const std::string & bus, float level);

    /**
     * @return the level a bus was last set to, or 1 for a bus never set and for no bus
     **/
    float busGain(const std::string & bus) const;

    void soundEvent(const v3d::event::kind::Sound & sound);

 private:
    /**
     * A track the mixer is playing, and what it was started as.
     **/
    struct Playing final {
        MIX_Track* track;
        std::string bus;
        float gain;  /**< the sound's own volume, before its bus's level scales it **/
        std::optional<glm::vec3> position;  /**< where a placed sound is in the world **/
    };

    /**
     * Give a track the gains of a pan. A centred pan clears the track's stereo gains instead
     * of setting both to one. A track reused after a panned sound then plays the next sound
     * exactly as an unpanned track would.
     **/
    static bool place(MIX_Track* track, float pan);

    /**
     * Give a track a position relative to the listener, from one in the world.
     **/
    bool place(MIX_Track* track, const glm::vec3& position) const;

    /**
     * Take the tracks of every finished sound back, so a game that starts one shots does
     * not grow a track per sound played.
     *
     * Called as a sound is started rather than on a timer, because a finished track costs
     * nothing until a new one is needed.
     **/
    void reap();

    /**
     * Take one finished voice's track back onto the free list.
     *
     * @return the entry after it
     **/
    std::map<Voice, Playing>::iterator retire(const std::map<Voice, Playing>::iterator & playing);

    /**
     * @return the voice's entry while its track is playing, or the end of voices_. A voice
     *         whose track has finished is retired here rather than at the next play().
     **/
    std::map<Voice, Playing>::iterator live(Voice voice);

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
    std::map<std::string, float> buses_;  /**< the level each bus was set to **/
    Listener listener_;
    boost::shared_ptr<Events> events_;  /**< the event backend that hears from listener_ **/
    std::vector<MIX_Track*> free_;   /**< reaped tracks, waiting to be played again **/
    uint32_t nextVoice_ = 1;         /**< never reused, so a stale handle stays stale **/
    // declared after dispatcher_, so it disconnects before the dispatcher is destroyed
    entt::scoped_connection sound_;
};

};  // namespace v3d::audio
