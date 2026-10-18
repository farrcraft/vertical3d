/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "System.h"

namespace v3d::ecs {

System::System(entt::registry* registry) : registry_(registry) {
}

entt::registry* System::registry() const noexcept {
    return registry_;
}

};  // namespace v3d::ecs
