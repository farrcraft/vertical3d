/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class String final : public Expression {
 public:
    String(const std::string & value, unsigned int line, unsigned int column);

    std::string value;
};

};  // namespace v3d::render::offline::sl::syntax
