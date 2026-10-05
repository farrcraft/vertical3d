/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <entt/entt.hpp>

namespace v3d::ecs {

class System {
 public:
    explicit System(entt::registry* registry);

    // a system is held polymorphically, so destruction has to reach the derived one
    virtual ~System() = default;

    /**
     * Advance this system by one simulation step.
     *
     * Named after Engine::simulate(), which calls it: a system runs once per fixed
     * simulation step, not once per drawn frame.
     *
     * @param step seconds of simulated time
     **/
    virtual bool simulate(float step) = 0;

 protected:
    entt::registry* registry_;
};

};  // namespace v3d::ecs
