/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jump.h"

namespace v3d::render::offline::sl::syntax {

Jump::Jump(Where where, const ExpressionPtr & value, const v3d::render::offline::Position & at) :
    Statement(Kind::JUMP, at),
    where(where),
    value(value) {
}

};  // namespace v3d::render::offline::sl::syntax
