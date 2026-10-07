/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>
#include <vector>

#include "Block.h"
#include "Parameter.h"

namespace v3d::render::offline::sl::syntax {

/**
 * A function defined inside a shader.
 *
 * The compiler rejects recursion by checking the call graph, because the machine has no
 * call stack.
 **/
class Function final {
 public:
    Type type = Type::VOID;
    std::string name;
    std::vector<Parameter> parameters;
    BlockPtr body;
    unsigned int line = 0;
    unsigned int column = 0;
};

};  // namespace v3d::render::offline::sl::syntax
