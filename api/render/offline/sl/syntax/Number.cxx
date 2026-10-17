/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Number.h"

namespace v3d::render::offline::sl::syntax {

Number::Number(float value, const v3d::render::offline::Position & at) :
    Expression(Kind::NUMBER, at),
    value(value) {
}

};  // namespace v3d::render::offline::sl::syntax
