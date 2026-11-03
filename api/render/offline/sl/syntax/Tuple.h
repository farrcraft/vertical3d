/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * A parenthesised list of two or more values, such as `(0, 0, 1)`. It is a literal of the type a
 * cast in front of it names, and is an error anywhere else.
 **/
class Tuple final : public Expression {
 public:
    Tuple(const v3d::render::offline::source::Position & at);

    std::vector<ExpressionPtr> elements;
};

};  // namespace v3d::render::offline::sl::syntax
