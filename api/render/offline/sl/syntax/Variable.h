/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Variable final : public Expression {
 public:
    Variable(const std::string & name, unsigned int line, unsigned int column);

    std::string name;
    /**
     * Which symbol the name resolved to, or -1 before Compiler has run. A local declared
     * twice in nested scopes is two symbols, so the index is what the machine allocates a
     * register against rather than the name.
     **/
    int symbol = -1;
};

};  // namespace v3d::render::offline::sl::syntax
