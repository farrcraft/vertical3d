/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Retained.h"

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

Retained::Retained() noexcept :
frame(0),
tab(0),
tabs(0),
scroll(0.0F),
content(0.0F),
offset(0.0F, 0.0F),
collapsed(false),
dragging(false),
chosen(-1) {
}

};  // namespace v3d::ui::immediate
