/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>

#include "Events.h"

#include <boost/shared_ptr.hpp>

namespace FMOD::Studio {
class EventInstance;
class System;
};  // namespace FMOD::Studio

namespace v3d::audio {

/**
 * The event backend over FMOD Studio. Built only when the build is configured with an FMOD
 * SDK, because the SDK cannot be redistributed.
 *
 * It owns one Studio system and the device FMOD opens for it, separate from the clip engine's.
 * Banks stay loaded until the backend is destroyed. An event start() started is kept until it
 * has stopped, and released by the next update() after that.
 *
 * An event no bank holds, and a global parameter FMOD will not set, are each logged once per
 * name for the life of the backend. Every later call for that name still returns false. A
 * caller that sets a parameter every frame therefore writes one warning, not one a frame.
 **/
class FmodEvents final : public Events {
 public:
    /**
     * Create and initialize the Studio system.
     *
     * @throw std::runtime_error when fmodstudio.dll or fmod.dll cannot be loaded, or when FMOD
     * cannot create or initialize the system
     **/
    explicit FmodEvents(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Release the system, and every bank and event instance with it.
     **/
    ~FmodEvents() override;

    FmodEvents(const FmodEvents&) = delete;
    FmodEvents& operator=(const FmodEvents&) = delete;

    bool bank(const std::string& path) override;
    bool play(std::string_view event) override;
    Instance start(std::string_view event) override;
    bool stop(Instance instance, Stop how) override;
    bool playing(Instance instance) const override;
    bool parameter(std::string_view name, float value) override;
    void update() override;
    std::string_view name() const noexcept override;

 private:
    /**
     * Find an event and make an instance of it, logging an event no bank holds once.
     *
     * @return the instance, not yet started, or null
     **/
    FMOD::Studio::EventInstance* create(const std::string& path);

    boost::shared_ptr<v3d::log::Logger> logger_;
    FMOD::Studio::System* system_;
    /**< events already reported missing. Kept apart from the parameters, so an event and a
     * parameter with the same name are each reported **/
    std::set<std::string, std::less<>> missingEvents_;
    /**< global parameters already reported as refused **/
    std::set<std::string, std::less<>> refusedParameters_;
    std::map<Instance, FMOD::Studio::EventInstance*> started_;  /**< what start() keeps hold of **/
    uint32_t nextInstance_ = 1;  /**< never reused, so a stale handle stays stale **/
};

};  // namespace v3d::audio
