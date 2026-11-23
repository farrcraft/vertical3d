/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * Whether a scrolled region puts up its scrollbar: a window decides at its start, from what
 * it drew last frame, and a table whenever there is more than it shows.
 **/
enum class Bar : std::uint8_t { Never, Always, WhenNeeded };

/**
 * A scrolled region. id holds its scroll and content, and scroll is the id its scrollbar is
 * hit tested under. contentTop is where its content would start if it were not scrolled.
 * bodyMax is the bottom right corner of its body, which the scrollbar runs down to.
 **/
struct Scrolled {
    Id id = 0;
    Id scroll = 0;
    float contentTop = 0.0F;
    glm::vec2 bodyMax{0.0F};
};

};  // namespace v3d::ui::immediate
