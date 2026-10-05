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
 * A formal parameter of a shader or of a function inside one.
 *
 * A shader parameter has a **required** default: SL has no uninitialised parameter, and a
 * scene that does not mention the parameter gets the default. A function's formals have
 * none, and that is the only difference between the two lists.
 **/
class Parameter final {
 public:
    Storage storage = Storage::UNSPECIFIED;
    /** Whether the shader writes it back, which only a shader parameter may be. **/
    bool output = false;
    Type type = Type::FLOAT;
    std::string name;
    ExpressionPtr defaultValue;
    unsigned int line = 0;
    unsigned int column = 0;
    /** The symbol Compiler gave it, or -1 before that pass has run. **/
    int symbol = -1;
};

};  // namespace v3d::render::offline::sl::syntax
