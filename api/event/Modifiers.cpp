/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Modifiers.h"

namespace v3d::event {

int Modifiers::count() const noexcept {
    return (ctrl ? 1 : 0) + (shift ? 1 : 0) + (alt ? 1 : 0);
}

bool Modifiers::within(const Modifiers& held) const noexcept {
    return (!ctrl || held.ctrl) && (!shift || held.shift) && (!alt || held.alt);
}

};  // namespace v3d::event
