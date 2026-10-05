/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Conditional.h"

namespace v3d::render::offline::sl::syntax {

Conditional::Conditional(const ExpressionPtr & condition, const StatementPtr & whenTrue,
    const StatementPtr & whenFalse, unsigned int line, unsigned int column) :
    Statement(Kind::CONDITIONAL, line, column),
    condition(condition),
    whenTrue(whenTrue),
    whenFalse(whenFalse) {
}

};  // namespace v3d::render::offline::sl::syntax
