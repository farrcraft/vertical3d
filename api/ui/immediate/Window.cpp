/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Window.h"

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

Window::Window() noexcept :
open(false),
margin(0.0F),
right(0.0F),
id(0),
scroll(0),
bodyMin(0.0F, 0.0F),
bodyMax(0.0F, 0.0F),
contentTop(0.0F),
scrolls(false),
clipped(false) {
}

};  // namespace v3d::ui::immediate
