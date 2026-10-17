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
 * The three message passing constructs. Each takes a parenthesised argument list and a body
 * that runs once per light, or once per surface point being lit.
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
