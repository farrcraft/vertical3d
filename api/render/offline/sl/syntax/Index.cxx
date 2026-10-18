/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Index.h"

#include <api/render/offline/Position.h>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

Index::Index(const Parts & parts, const v3d::render::offline::Position & at) :
    Expression(Kind::INDEX, at),
    array(parts.array),
    index(parts.index) {
}

};  // namespace v3d::render::offline::sl::syntax
