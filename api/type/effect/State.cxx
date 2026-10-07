/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "State.h"

#include <cstdint>

namespace v3d::type::effect {

State::State(uint64_t seed) noexcept :
    owed(0.0f),
    random(seed) {
}

};  // namespace v3d::type::effect
