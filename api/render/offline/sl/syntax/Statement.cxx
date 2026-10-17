/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

Statement::Statement(Kind kind, const v3d::render::offline::Position & at) :
    kind(kind),
    line(at.line),
    column(at.column) {
}

Statement::~Statement() {
}

};  // namespace v3d::render::offline::sl::syntax
