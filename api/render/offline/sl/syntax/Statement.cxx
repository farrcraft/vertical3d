/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

Statement::Statement(Kind kind, unsigned int line, unsigned int column) :
    kind(kind),
    line(line),
    column(column) {
}

Statement::~Statement() {
}

};  // namespace v3d::render::offline::sl::syntax
