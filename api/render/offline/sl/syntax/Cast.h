/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * A typecast, which is also how a scene names a coordinate space:
 * `point "world" (0, 0, 0)` casts and transforms in one. The space is empty when none was
 * given, which means the shader's current space.
 **/
class Cast final : public Expression {
 public:
    Cast(Type type, const std::string & space, const ExpressionPtr & operand,
        unsigned int line, unsigned int column);

    Type type = Type::FLOAT;
    std::string space;
    ExpressionPtr operand;
};

};  // namespace v3d::render::offline::sl::syntax
