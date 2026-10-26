/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "Listener.h"

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::audio {

/**
 * An event that Events::start() started, for as long as it plays.
 *
 * An id, not a pointer. A handle to an event that has stopped and been released is refused,
 * and is never confused with a newer event. None is no event at all, and a failed start()
 * returns it.
 **/
// NOLINTNEXTLINE(performance-enum-size)
enum class Instance : uint32_t { None = 0 };

/**
 * How Events::stop() ends an event.
 **/
enum class Stop : uint8_t {
    FadeOut,     /**< play the fade out the event was authored with, so it ends without a click **/
    Immediately  /**< cut it off **/
};

/**
 * Sound authored in an audio tool: banks, the named events in them, and the global parameters
 * their mix reads.
 *
 * It works beside the clip engine and does not replace it. Clips, buses and fades stay on
 * audio::Engine. An app that plays events gets one of these from events(). In a build with no
 * event backend, events() returns the null backend, so the app builds and runs either way.
 *
 * Every call returns whether it succeeded. The null backend does nothing and returns false or
 * nothing, and a game treats that as silence, not as an error.
 **/
class Events {
 public:
    virtual ~Events() = default;

    /**
     * Load a bank, after which the events in it can be played.
     *
     * @param path the bank's file, as the platform opens it
     * @return whether the bank loaded
     **/
    virtual bool bank(const std::string& path) = 0;

    /**
     * Start an event as a one shot: it plays to its end and is released.
     *
     * @param event the event's path, such as "event:/UI/Cancel"
     * @return whether it started, which is false for an event no loaded bank holds
     **/
    virtual bool play(std::string_view event) = 0;

    /**
     * Start an event and keep hold of it, so that it can be stopped. An event authored to
     * loop plays until it is stopped, and one that is not plays to its end.
     *
     * @param event the event's path, such as "event:/Ambience/Country"
     * @return the instance, or Instance::None for an event no loaded bank holds and on the
     *         null backend
     **/
    virtual Instance start(std::string_view event) = 0;

    /**
     * Start an event as a one shot at a point in the world. An event authored as 2D ignores
     * the position.
     *
     * @param position where the event plays, in the coordinates listener() is given
     * @return whether it started
     **/
    virtual bool play(std::string_view event, const glm::vec3& position) = 0;

    /**
     * Start an event at a point in the world and keep hold of it, so that it can be moved with
     * position() and stopped. The position is set before the event starts, so it is never heard
     * from anywhere else.
     *
     * @return the instance, or Instance::None as start() returns it
     **/
    virtual Instance start(std::string_view event, const glm::vec3& position) = 0;

    /**
     * Move an event that start() started.
     *
     * @return false for an instance that is not playing
     **/
    virtual bool position(Instance instance, const glm::vec3& position) = 0;

    /**
     * @return where an event is, as the backend holds it, or nothing for an instance that is
     *         not playing. An event started with no position is at the origin
     **/
    virtual std::optional<glm::vec3> position(Instance instance) const = 0;

    /**
     * Set where events are heard from. audio::Engine::listener() calls this for an event
     * backend it was given, so a game sets one listener for clips and events.
     *
     * @param listener a listener orthonormal() returned
     * @return whether the backend took it, which the null backend never does
     **/
    virtual bool listener(const Listener& listener) = 0;

    /**
     * Stop an event that start() started. A fade out keeps it playing until the fade ends.
     * The instance is released by update() once it has stopped.
     *
     * @return false for an instance that is not playing, which a released one is
     **/
    virtual bool stop(Instance instance, Stop how) = 0;

    /**
     * @return whether the event is still playing, which it is while it fades out. False once
     *         it has stopped, and for Instance::None
     **/
    virtual bool playing(Instance instance) const = 0;

    /**
     * Set a global parameter, which every event that reads it follows.
     *
     * @return whether the parameter exists and took the value
     **/
    virtual bool parameter(std::string_view name, float value) = 0;

    /**
     * Set how loud a bus or a VCA plays, which is what a player's volume slider moves. The path
     * says which it is: "bus:/" is the master bus, "bus:/Music" a bus under it, and
     * "vca:/SFX" a VCA. A VCA scales the buses assigned to it without changing their routing.
     *
     * A path is found through the strings bank, so the bank that holds the names has to be
     * loaded first. A level set before then is refused, and the app sets it again afterwards.
     *
     * @param level 1 as authored and 0 silent. A level below 0 is taken as 0
     * @return whether the path exists and took the level
     **/
    virtual bool volume(std::string_view path, float level) = 0;

    /**
     * @return the level a bus or a VCA was set to, or nothing for a path no loaded bank holds
     **/
    virtual std::optional<float> volume(std::string_view path) const = 0;

    /**
     * Pause or resume a bus and everything routed through it. Pausing "bus:/" pauses
     * everything. Pausing the game's buses and not the menu's is a pause menu. A VCA has no
     * pause, so a "vca:/" path is refused.
     *
     * @return whether the bus exists and took the change
     **/
    virtual bool paused(std::string_view bus, bool paused) = 0;

    /**
     * @return whether a bus is paused, or nothing for a path no loaded bank holds
     **/
    virtual std::optional<bool> paused(std::string_view bus) const = 0;

    /**
     * Let the backend do a frame's work, and release every started event that has stopped.
     * The app calls this once a frame, from tick().
     **/
    virtual void update() = 0;

    /**
     * @return the backend's name, for the log: "null" or "fmod"
     **/
    virtual std::string_view name() const noexcept = 0;
};

/**
 * The event backend this build has: FMOD Studio when the build was configured with an SDK and
 * it starts, and the null backend otherwise. A backend that fails to start is logged, and the
 * null one is returned in its place.
 **/
boost::shared_ptr<Events> events(const boost::shared_ptr<v3d::log::Logger>& logger);

};  // namespace v3d::audio
