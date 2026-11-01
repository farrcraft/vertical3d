/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DividerHint.h"

#include <optional>

namespace v3d::editor::control {

/**
 **/
bool DividerHint::due(const Seen& seen, float now) {
    if (!seen.lit || seen.held) {
        since_.reset();
        return false;
    }
    if (!since_) {
        since_ = now;
    }
    return now - *since_ >= DELAY;
}

};  // namespace v3d::editor::control
