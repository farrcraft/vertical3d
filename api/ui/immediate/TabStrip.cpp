/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "TabStrip.h"

namespace v3d::ui::immediate {

TabStrip::TabStrip() noexcept :
id(0),
open(false),
pen(0.0F),
top(0.0F),
share(0.0F),
index(0),
wanted(0),
changed(false),
taken(false) {
}

};  // namespace v3d::ui::immediate
