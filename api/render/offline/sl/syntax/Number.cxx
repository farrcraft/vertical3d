/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Number.h"

namespace v3d::render::offline::sl::syntax {

Number::Number(float value, unsigned int line, unsigned int column) :
    Expression(Kind::NUMBER, line, column),
    value(value) {
}

};  // namespace v3d::render::offline::sl::syntax
