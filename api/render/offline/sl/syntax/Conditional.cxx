/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Conditional.h"

#include <api/render/offline/Position.h>

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

Conditional::Conditional(const ExpressionPtr & condition, const Branches & branches, const v3d::render::offline::Position & at) :
    Statement(Kind::CONDITIONAL, at),
    condition(condition),
    whenTrue(branches.whenTrue),
    whenFalse(branches.whenFalse) {
}

};  // namespace v3d::render::offline::sl::syntax
