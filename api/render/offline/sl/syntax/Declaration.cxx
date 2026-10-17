/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Declaration.h"

namespace v3d::render::offline::sl::syntax {

Declaration::Declaration(Storage storage, Type type, const v3d::render::offline::Position & at) :
    Statement(Kind::DECLARATION, at),
    storage(storage),
    type(type) {
}

};  // namespace v3d::render::offline::sl::syntax
