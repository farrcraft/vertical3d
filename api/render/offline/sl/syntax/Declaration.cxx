/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Declaration.h"

namespace v3d::render::offline::sl::syntax {

Declaration::Declaration(Storage storage, Type type, unsigned int line, unsigned int column) :
    Statement(Kind::DECLARATION, line, column),
    storage(storage),
    type(type) {
}

};  // namespace v3d::render::offline::sl::syntax
