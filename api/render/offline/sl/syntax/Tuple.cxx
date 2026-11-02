/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tuple.h"

#include <api/render/offline/source/Position.h>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

Tuple::Tuple(const v3d::render::offline::source::Position & at) :
    Expression(Kind::TUPLE, at) {
}

};  // namespace v3d::render::offline::sl::syntax
