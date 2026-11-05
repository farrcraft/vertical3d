/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * A number literal. SL has no integer type, so every number is a float.
 **/
class Number final : public Expression {
 public:
    Number(float value, const v3d::render::offline::source::Position & at);

    float value = 0.0F;
};

};  // namespace v3d::render::offline::sl::syntax
