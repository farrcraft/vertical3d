/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>
#include <vector>

#include "Instruction.h"
#include "Register.h"

namespace v3d::render::offline::sl::runtime {

/**
 * A compiled shader: its registers and the instructions over them.
 *
 * The first `symbols` registers are the compiler's symbols in its own order - globals, then
 * the shader's parameters, then the locals - so a renderer binds a parameter or reads Ci by
 * the index the compiler gave it. Everything after them is a temporary or a constant.
 **/
class Program final {
 public:
    ShaderType type = ShaderType::SURFACE;
    std::string name;
    std::vector<Register> registers;
    std::vector<Instruction> instructions;
    /** How many of the registers are symbols. **/
    std::size_t symbols = 0;
    /**
     * How many of the instructions compute the shader's declared parameter defaults.
     *
     * They are at the front and a run starts after them, because a default written as an
     * instruction the body runs would overwrite the value a scene bound on every pass over
     * a grid. `Shader` runs them once and reads the answers out.
     **/
    std::size_t prologue = 0;

    /**
     * The register a named symbol is, or -1. What a renderer binds a parameter or reads Ci
     * through, when it did not keep the index the compiler gave it.
     **/
    int symbol(const std::string & wanted) const;
};

};  // namespace v3d::render::offline::sl::runtime
