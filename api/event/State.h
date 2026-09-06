/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::event {
/**
 * Whether an event is a press or a release.
 *
 * Source events carry the state they actually occurred in - Pressed or Released.
 * A binding carries the state it wants to match: Pressed or Released to bind one edge
 * only, or Any to bind both. A destination event is given the state of the source event
 * that triggered it, so a handler can tell the two edges apart without needing a
 * separate binding for each.
 **/
enum class State {
    Any,
    Pressed,
    Released
};

/**
 * Convert a binding config's state name to a State.
 * An unrecognized name - including the absence of one - is Any, which matches both edges.
 **/
constexpr State stringToState(const std::string_view& stateName) {
    if (stateName == "pressed" || stateName == "down") {
        return State::Pressed;
    } else if (stateName == "released" || stateName == "up") {
        return State::Released;
    }
    return State::Any;
}
};  // namespace v3d::event
