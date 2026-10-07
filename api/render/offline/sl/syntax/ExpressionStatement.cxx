/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ExpressionStatement.h"

namespace v3d::render::offline::sl::syntax {

ExpressionStatement::ExpressionStatement(const ExpressionPtr & expression,
    unsigned int line, unsigned int column) :
    Statement(Kind::EXPRESSION, line, column),
    expression(expression) {
}

};  // namespace v3d::render::offline::sl::syntax
