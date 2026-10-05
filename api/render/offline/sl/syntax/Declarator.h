/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * One name of a declaration, with the expression that initialises it or none.
 **/
class Declarator final {
 public:
    std::string name;
    ExpressionPtr initialiser;
    unsigned int line = 0;
    unsigned int column = 0;
    /** The symbol Compiler gave it, or -1 before that pass has run. **/
    int symbol = -1;
};

};  // namespace v3d::render::offline::sl::syntax
