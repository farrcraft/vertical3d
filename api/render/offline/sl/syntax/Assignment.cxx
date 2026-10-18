/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Assignment.h"

#include <api/render/offline/Position.h>

#include "Statement.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Assignment::Assignment(const std::string & op, const Sides & sides, const v3d::render::offline::Position & at) :
    Statement(Kind::ASSIGNMENT, at),
    op(op),
    target(sides.target),
    value(sides.value) {
}

};  // namespace v3d::render::offline::sl::syntax
