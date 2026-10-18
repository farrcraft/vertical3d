/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Ternary.h"

#include <api/render/offline/Position.h>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

Ternary::Ternary(const Branches & branches, const v3d::render::offline::Position & at) :
    Expression(Kind::TERNARY, at),
    condition(branches.condition),
    whenTrue(branches.whenTrue),
    whenFalse(branches.whenFalse) {
}

};  // namespace v3d::render::offline::sl::syntax
