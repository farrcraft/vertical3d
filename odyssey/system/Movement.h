/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../api/ecs/System.h"

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
};
};  // namespace odyssey::system
