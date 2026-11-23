/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <optional>
#include <string>

#include "Problem.h"

namespace v3d::script {

/**
 * TypeScript with its types taken out, or why they could not be.
 **/
struct Stripped final {
    std::string code;                   /**< the JavaScript left, the source's length and lines **/
    std::optional<Problem> problem;     /**< set, with no code, when the source was refused **/
};

};  // namespace v3d::script
