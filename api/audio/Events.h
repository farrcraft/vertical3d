/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

namespace v3d::audio {

/**
 * Sound authored in an audio tool: banks, the named events in them, and the global parameters
 * their mix reads.
 *
 * It sits beside the clip engine rather than replacing it. Clips, buses and fades stay on
 * audio::Engine. An app that plays events asks events() for one of these, and gets the null
 * backend where no event backend was built, so it builds and runs either way.
 *
 * Every call reports whether it did what it was asked. The null backend does nothing and says
 * so, which a game reads as silence rather than as an error.
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
     * Set a global parameter, which every event that reads it follows.
     *
     * @return whether the parameter exists and took the value
     **/
    virtual bool parameter(std::string_view name, float value) = 0;

    /**
     * Let the backend do a frame's work. The app calls this once a frame, from tick().
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
