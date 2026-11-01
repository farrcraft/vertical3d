/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DisplayScale.h"

namespace v3d::event::kind {

DisplayScale::DisplayScale(float scale) noexcept : scale_(scale) {
}

float DisplayScale::scale() const noexcept {
    return scale_;
}
};  // namespace v3d::event::kind
