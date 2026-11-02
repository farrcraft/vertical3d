/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace v3d::editor::control::chrome {

/**
 * The hint a divider shows: how a drag, a double click, Escape and Ctrl act on it.
 *
 * It is due once a divider has been lit under the cursor for as long as a tooltip waits, and
 * goes as soon as the divider is held or no longer lit. Times are seconds on the ui's clock.
 **/
class DividerHint final {
 public:
    /**
     * How long a divider is lit before its hint shows, as long as a tooltip waits.
     **/
    static constexpr float DELAY = 0.5F;

    /**
     * What the hint says, one line each: a heading and how each way of acting on the divider acts.
     **/
    static constexpr std::array<std::string_view, 2> LINES = { "Drag to resize",
        "Double-click to even, Escape to cancel, Ctrl to snap to tenths" };

    /**
     * What the pointer is doing to the dividers.
     **/
    struct Seen final {
        bool lit = false;    /**< whether a divider is lit under the cursor **/
        bool held = false;   /**< whether a divider is held **/
    };

    /**
     * @param now the ui clock's time
     * @return whether the hint shows now
     **/
    bool due(const Seen& seen, float now);

 private:
    // when the divider was first lit, while it stays lit and is not held
    std::optional<float> since_;
};

};  // namespace v3d::editor::control::chrome
