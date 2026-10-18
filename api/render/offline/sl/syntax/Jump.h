/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"
#include "Statement.h"

#include <cstdint>

namespace v3d::render::offline::sl::syntax {

class Jump final : public Statement {
 public:
    enum class Where : std::uint8_t {
        BREAK,
        CONTINUE,
        RETURN
    };

    Jump(Where where, const ExpressionPtr & value, const v3d::render::offline::Position & at);

    Where where = Where::RETURN;
    /** What a return returns, or null for a bare one and for the other two. **/
    ExpressionPtr value;
};

};  // namespace v3d::render::offline::sl::syntax
