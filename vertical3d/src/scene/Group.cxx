/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Group.h"

#include "Item.h"

namespace v3d::editor {

/**
 **/
Group::Group() {
}

/**
 **/
Item::Kind Group::kind() const noexcept {
    return KIND;
}

};  // namespace v3d::editor
