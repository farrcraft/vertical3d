/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/ecs/System.h>

namespace odyssey::system {
/**
 **/
class Movement final : public v3d::ecs::System {
 public:
    /**
     * Inherit base constructor
     **/
    using System::System;

    /**
     * @return bool
     **/
    bool simulate(float step) override;

 private:
    /**
     * How long an entity spends crossing one tile, in seconds. On the grid a diagonal step
     * costs the same as an orthogonal one, so it takes the same time. A walk therefore keeps a
     * steady pace rather than slowing down around corners.
     **/
    static constexpr float secondsPerTile = 0.15F;
};
};  // namespace odyssey::system
