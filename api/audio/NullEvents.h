/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

#include "Events.h"

namespace v3d::audio {

/**
 * The event backend of a build without one. It loads nothing, plays nothing and sets nothing,
 * and returns false from each, so a game that plays events runs silent rather than failing.
 **/
class NullEvents final : public Events {
 public:
    bool bank(const std::string& path) override;
    bool play(std::string_view event) override;
    bool parameter(std::string_view name, float value) override;
    void update() override;
    std::string_view name() const noexcept override;
};

};  // namespace v3d::audio
