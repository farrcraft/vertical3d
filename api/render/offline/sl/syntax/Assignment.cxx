/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Assignment.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Assignment::Assignment(const std::string & op, const ExpressionPtr & target,
    const ExpressionPtr & value, unsigned int line, unsigned int column) :
    Statement(Kind::ASSIGNMENT, line, column),
    op(op),
    target(target),
    value(value) {
}

};  // namespace v3d::render::offline::sl::syntax
