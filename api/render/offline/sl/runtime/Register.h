/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>
#include <vector>

namespace v3d::render::offline::sl::runtime {

/**
 * One value the program holds, sized once when the machine is prepared.
 **/
class Register final {
 public:
    Type type = Type::FLOAT;
    Storage storage = Storage::UNIFORM;
    /** The symbol's name, or empty for a temporary. **/
    std::string name;
    /**
     * Whether the symbol is a shader parameter. A scene may bind a parameter, and the
     * prologue leaves it holding its declared default.
     **/
    bool parameter = false;
    /**
     * Whether the register holds a literal the machine writes once at the start of a run
     * rather than something the program computes.
     **/
    bool constant = false;
    std::vector<float> value;
    std::string text;
};

};  // namespace v3d::render::offline::sl::runtime
