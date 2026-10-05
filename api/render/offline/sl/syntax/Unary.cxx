/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Unary.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Unary::Unary(const std::string & op, const ExpressionPtr & operand, unsigned int line, unsigned int column) :
    Expression(Kind::UNARY, line, column),
    op(op),
    operand(operand) {
}

};  // namespace v3d::render::offline::sl::syntax
