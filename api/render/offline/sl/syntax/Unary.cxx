/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Unary.h"

#include <api/render/offline/source/Position.h>

#include "Expression.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Unary::Unary(const std::string & op, const ExpressionPtr & operand, const v3d::render::offline::source::Position & at) :
    Expression(Kind::UNARY, at),
    op(op),
    operand(operand) {
}

};  // namespace v3d::render::offline::sl::syntax
