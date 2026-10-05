/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Cast.h"

#include <string>

namespace v3d::render::offline::sl::syntax {

Cast::Cast(Type type, const std::string & space, const ExpressionPtr & operand,
    unsigned int line, unsigned int column) :
    Expression(Kind::CAST, line, column),
    type(type),
    space(space),
    operand(operand) {
}

};  // namespace v3d::render::offline::sl::syntax
