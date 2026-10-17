/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Binary.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Binary::Binary(const std::string & op, const Operands & operands, const v3d::render::offline::Position & at) :
    Expression(Kind::BINARY, at),
    op(op),
    left(operands.left),
    right(operands.right) {
}

};  // namespace v3d::render::offline::sl::syntax
