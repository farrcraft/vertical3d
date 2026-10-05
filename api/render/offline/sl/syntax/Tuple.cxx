/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tuple.h"

namespace v3d::render::offline::sl::syntax {

Tuple::Tuple(unsigned int line, unsigned int column) :
    Expression(Kind::TUPLE, line, column) {
}

};  // namespace v3d::render::offline::sl::syntax
