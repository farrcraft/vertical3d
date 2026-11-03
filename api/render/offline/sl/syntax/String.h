/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * A string literal, with its escapes resolved and without its quotes.
 **/
class String final : public Expression {
 public:
    String(const std::string & value, const v3d::render::offline::source::Position & at);

    std::string value;
};

};  // namespace v3d::render::offline::sl::syntax
