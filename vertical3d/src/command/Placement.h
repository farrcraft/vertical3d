/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

namespace v3d::editor {

/**
 * Whether two placements are the same to within a tolerance.
 *
 * A manipulator writes one of translation, rotation and scale on an item. A gesture is undone
 * by restoring the item's whole placement, so the record does not depend on which manipulator
 * made it. A gesture that measured nothing still composes a rotation and adds a zero offset, so
 * the two ends of it differ in the last bits without the item having moved.
 **/
bool same(const v3d::type::Transform& first, const v3d::type::Transform& second);

};  // namespace v3d::editor
