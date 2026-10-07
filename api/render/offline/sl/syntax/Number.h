/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Number final : public Expression {
 public:
    Number(float value, unsigned int line, unsigned int column);

    float value = 0.0f;
};

};  // namespace v3d::render::offline::sl::syntax
