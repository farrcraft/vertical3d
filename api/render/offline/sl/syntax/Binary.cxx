/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Binary.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Binary::Binary(const std::string & op, const ExpressionPtr & left, const ExpressionPtr & right,
    unsigned int line, unsigned int column) :
    Expression(Kind::BINARY, line, column),
    op(op),
    left(left),
    right(right) {
}

};  // namespace v3d::render::offline::sl::syntax
