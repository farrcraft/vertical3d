/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Expression.h"

#include <api/render/offline/Position.h>

namespace v3d::render::offline::sl::syntax {

Expression::Expression(Kind kind, const v3d::render::offline::Position & at) :
    kind(kind),
    line(at.line),
    column(at.column) {
}

Expression::~Expression() {
}

};  // namespace v3d::render::offline::sl::syntax
