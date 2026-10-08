/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <string_view>

#include "Events.h"

#include <boost/shared_ptr.hpp>

namespace FMOD::Studio {
class System;
};  // namespace FMOD::Studio

namespace v3d::audio {

/**
 * The event backend over FMOD Studio. Built only when the build is configured with an FMOD
 * SDK, because the SDK cannot be redistributed.
 *
 * It owns one Studio system and the device FMOD opens for it, separate from the clip engine's.
 * Banks stay loaded until the backend is destroyed.
 **/
class FmodEvents final : public Events {
 public:
    /**
     * Create and initialize the Studio system.
     *
     * @throw std::runtime_error when FMOD cannot create or initialize it
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
    bool parameter(std::string_view name, float value) override;
    void update() override;
    std::string_view name() const noexcept override;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    FMOD::Studio::System* system_;
};

};  // namespace v3d::audio
