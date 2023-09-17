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

        virtual bool tick() = 0;

     protected:
        entt::registry* registry_;
    };

};  // namespace v3d::ecs
