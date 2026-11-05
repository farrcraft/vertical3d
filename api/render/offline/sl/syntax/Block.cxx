/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Block.h"

#include <api/render/offline/source/Position.h>

#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

Block::Block(const v3d::render::offline::source::Position & at) :
    Statement(Kind::BLOCK, at) {
}

};  // namespace v3d::render::offline::sl::syntax
