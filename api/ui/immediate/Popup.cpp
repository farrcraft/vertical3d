/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Popup.h"

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

Popup::Popup() noexcept :
id(0),
asked(false),
boxMin(0.0F, 0.0F),
boxMax(0.0F, 0.0F),
selected(-1) {
}

};  // namespace v3d::ui::immediate
