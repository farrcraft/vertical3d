/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jump.h"

namespace v3d::render::offline::sl::syntax {

Jump::Jump(Where where, const ExpressionPtr & value, unsigned int line, unsigned int column) :
    Statement(Kind::JUMP, line, column),
    where(where),
    value(value) {
}

};  // namespace v3d::render::offline::sl::syntax
