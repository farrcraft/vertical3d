/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Tuple final : public Expression {
 public:
    Tuple(const v3d::render::offline::Position & at);

    std::vector<ExpressionPtr> elements;
};

};  // namespace v3d::render::offline::sl::syntax
