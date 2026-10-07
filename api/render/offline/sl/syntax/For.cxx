/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "For.h"

namespace v3d::render::offline::sl::syntax {

For::For(unsigned int line, unsigned int column) :
    Statement(Kind::FOR, line, column) {
}

};  // namespace v3d::render::offline::sl::syntax
