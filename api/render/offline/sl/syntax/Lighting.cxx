/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lighting.h"

namespace v3d::render::offline::sl::syntax {

Lighting::Lighting(Construct construct, const v3d::render::offline::Position & at) :
    Statement(Kind::LIGHTING, at),
    construct(construct) {
}

};  // namespace v3d::render::offline::sl::syntax
