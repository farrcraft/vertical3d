/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "While.h"

namespace v3d::render::offline::sl::syntax {

While::While(const ExpressionPtr & condition, const StatementPtr & body,
    unsigned int line, unsigned int column) :
    Statement(Kind::WHILE, line, column),
    condition(condition),
    body(body) {
}

};  // namespace v3d::render::offline::sl::syntax
