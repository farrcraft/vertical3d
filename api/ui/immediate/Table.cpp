/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Table.h"

namespace v3d::ui::immediate {

Table::Table() noexcept :
open(false),
left(0.0F),
column(0),
id(0),
scroll(0),
right(0.0F),
height(0.0F),
top(0.0F),
contentTop(0.0F),
clipped(false) {
}

};  // namespace v3d::ui::immediate
