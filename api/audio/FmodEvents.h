/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "Events.h"
#include "Listener.h"

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

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
 * The system is started right-handed, so a position and a listener are handed to FMOD in the
 * world's own axes. A mirrored listener has its right at up x forward, which FMOD cannot be
 * told. For one, the backend reflects the world through x = 0 on the way in and out. FMOD then
 * hears every distance and side as the game draws it.
 *
 * Banks stay loaded until the backend is destroyed. An event start() started is kept until it
 * has stopped, and released by the next update() after that.
 *
 * An event no bank holds, a global parameter FMOD will not set, and a bus or VCA path no bank
 * holds, are each logged once per name for the life of the backend. Every later call for that name still returns false. A
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
    bool play(std::string_view event, const glm::vec3& position) override;
    Instance start(std::string_view event, const glm::vec3& position) override;
    bool position(Instance instance, const glm::vec3& position) override;
    std::optional<glm::vec3> position(Instance instance) const override;
    bool listener(const Listener& listener) override;
    bool stop(Instance instance, Stop how) override;
    bool playing(Instance instance) const override;
    bool parameter(std::string_view name, float value) override;
    bool volume(std::string_view path, float level) override;
    std::optional<float> volume(std::string_view path) const override;
    bool paused(std::string_view bus, bool paused) override;
    std::optional<bool> paused(std::string_view bus) const override;
    void update() override;
    std::string_view name() const noexcept override;

 private:
    /**
     * Find an event and make an instance of it, logging an event no bank holds once.
     *
     * @param position where to place it before it starts, or nothing to leave it at the origin
     * @return the instance, not yet started, or null
     **/
    FMOD::Studio::EventInstance* create(const std::string& path, const std::optional<glm::vec3>& position);

    /**
     * Start a one shot, placed or not, and release it to FMOD.
     **/
    bool oneShot(std::string_view event, const std::optional<glm::vec3>& position);

    /**
     * Start an event, placed or not, and keep hold of it.
     **/
    Instance keep(std::string_view event, const std::optional<glm::vec3>& position);

    /**
     * A point in the world as FMOD holds it, and back again, since a reflection is its own
     * inverse.
     **/
    glm::vec3 reflect(const glm::vec3& point) const noexcept;

    /**
     * Log a bus or VCA path FMOD refused, once per path.
     **/
    void refuse(const std::string& path, const char* why);

    boost::shared_ptr<v3d::log::Logger> logger_;
    FMOD::Studio::System* system_;
    /**< events already reported missing. Kept apart from the parameters, so an event and a
     * parameter with the same name are each reported **/
    std::set<std::string, std::less<>> missingEvents_;
    /**< global parameters already reported as refused **/
    std::set<std::string, std::less<>> refusedParameters_;
    /**< bus and VCA paths already reported as refused **/
    std::set<std::string, std::less<>> refusedMixes_;
    std::map<Instance, FMOD::Studio::EventInstance*> started_;  /**< what start() keeps hold of **/
    uint32_t nextInstance_ = 1;  /**< never reused, so a stale handle stays stale **/
    bool mirrored_ = false;      /**< whether the last listener was mirrored **/
};

};  // namespace v3d::audio
