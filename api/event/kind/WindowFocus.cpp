/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowFocus.h"

namespace v3d::event::kind {

WindowFocus::WindowFocus(bool gained) noexcept : gained_(gained) {
}

bool WindowFocus::gained() const noexcept {
    return gained_;
}
};  // namespace v3d::event::kind
