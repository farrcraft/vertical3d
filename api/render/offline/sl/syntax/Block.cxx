/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Block.h"

namespace v3d::render::offline::sl::syntax {

Block::Block(unsigned int line, unsigned int column) :
    Statement(Kind::BLOCK, line, column) {
}

};  // namespace v3d::render::offline::sl::syntax
