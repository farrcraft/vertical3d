/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Ternary.h"

namespace v3d::render::offline::sl::syntax {

Ternary::Ternary(const ExpressionPtr & condition, const ExpressionPtr & whenTrue,
    const ExpressionPtr & whenFalse, unsigned int line, unsigned int column) :
    Expression(Kind::TERNARY, line, column),
    condition(condition),
    whenTrue(whenTrue),
    whenFalse(whenFalse) {
}

};  // namespace v3d::render::offline::sl::syntax
