/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Row.h"

namespace v3d::ui::immediate {

Row::Row() noexcept :
margin(0.0F),
right(0.0F),
penY(0.0F),
top(0.0F),
height(0.0F),
lastRight(0.0F),
sameLine(false) {
}

};  // namespace v3d::ui::immediate
