/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Call final : public Expression {
 public:
    Call(const std::string & name, unsigned int line, unsigned int column);

    std::string name;
    std::vector<ExpressionPtr> arguments;
    /**
     * Which of the shader's own functions this calls, or -1 for one of the standard
     * library's. Filled in by Compiler.
     **/
    int function = -1;
    /**
     * Which signature of the standard library it matched, as an index into `builtins()`,
     * or -1. An index rather than the signature itself, because Builtins.h declares the
     * signatures in terms of these nodes, and a node naming one would make the two circular.
     **/
    int signature = -1;
};

};  // namespace v3d::render::offline::sl::syntax
