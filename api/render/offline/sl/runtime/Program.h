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
 * The first `symbols` registers are the compiler's symbols, in the compiler's order: globals,
 * then the shader's parameters, then the locals. A renderer binds a parameter or reads Ci by
 * the index the compiler gave it. Every later register is a temporary or a constant.
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
     * They are at the front, and Machine::run() starts after them. Run on every grid, they
     * would overwrite the values a scene bound. `Machine::initialise()` runs them once,
     * before a scene's values are written over the defaults.
     **/
    std::size_t prologue = 0;
    /**
     * Whether the shader calls `trace` or `transmission`, and so needs the renderer's traced
     * scene. A shader's own functions are inlined, so a call through one counts.
     **/
    bool traces = false;
    /**
     * Whether the shader reads a derivative: it calls `Du`, `Dv`, `Deriv`, `area` or
     * `calculatenormal`, or reads `du` or `dv`. A renderer shades the neighbours a derivative
     * differences only for a program that does. Its own functions are inlined, so a call
     * through one counts.
     **/
    bool differentiates = false;

    /**
     * The register holding a named symbol, or -1. A renderer that did not keep the index
     * the compiler gave a symbol uses this to bind a parameter or read Ci.
     **/
    int symbol(const std::string & wanted) const;
};

};  // namespace v3d::render::offline::sl::runtime
