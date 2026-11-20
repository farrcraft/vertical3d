/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::event {

class Source;

/**
 * A source no listener consumed, for the event router to turn into the commands it is bound
 * to. Only the event router listens for this.
 **/
struct Unclaimed final {
    const Source* source;  // never null. A pointer, because a dispatcher stores its events by value
};

};  // namespace v3d::event
