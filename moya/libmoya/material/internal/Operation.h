/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Op.h>

#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::material::internal {

/** The instruction a MaterialX node compiles to, and the inputs it reads as operands, in order. **/
struct Operation {
    kernel::program::Op op { kernel::program::Op::Copy };
    std::vector<std::string> inputs;

    /**
     * The operation of a node of a category, such as add or mix, or nothing for a category the
     * program does not compile.
     **/
    static std::optional<Operation> of(const std::string & category);
};

};  // namespace v3d::moya::material::internal
