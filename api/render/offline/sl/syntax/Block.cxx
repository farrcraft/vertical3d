/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Block.h"

namespace v3d::render::offline::sl::syntax {

Block::Block(const v3d::render::offline::Position & at) :
    Statement(Kind::BLOCK, at) {
}

};  // namespace v3d::render::offline::sl::syntax
