/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

Expression::Expression(Kind kind, unsigned int line, unsigned int column) :
    kind(kind),
    line(line),
    column(column) {
}

Expression::~Expression() {
}

};  // namespace v3d::render::offline::sl::syntax
