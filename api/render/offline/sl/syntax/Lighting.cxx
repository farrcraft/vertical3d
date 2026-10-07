/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lighting.h"

namespace v3d::render::offline::sl::syntax {

Lighting::Lighting(Construct construct, unsigned int line, unsigned int column) :
    Statement(Kind::LIGHTING, line, column),
    construct(construct) {
}

};  // namespace v3d::render::offline::sl::syntax
