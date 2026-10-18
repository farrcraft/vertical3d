/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <vector>

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

/**
 * illuminance, illuminate or solar. Each takes a parenthesised argument list and a body.
 * An illuminance body runs once per light. An illuminate or solar body runs over the
 * surface points the light reaches.
 **/
class Lighting final : public Statement {
 public:
    enum class Construct : std::uint8_t {
        ILLUMINANCE,
        ILLUMINATE,
        SOLAR
    };

    Lighting(Construct construct, const v3d::render::offline::Position & at);

    Construct construct = Construct::ILLUMINANCE;
    std::vector<ExpressionPtr> arguments;
    StatementPtr body;
};

};  // namespace v3d::render::offline::sl::syntax
