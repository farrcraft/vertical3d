/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ExpressionStatement.h"

#include <api/render/offline/Position.h>

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

ExpressionStatement::ExpressionStatement(const ExpressionPtr & expression,
    const v3d::render::offline::Position & at) :
    Statement(Kind::EXPRESSION, at),
    expression(expression) {
}

};  // namespace v3d::render::offline::sl::syntax
