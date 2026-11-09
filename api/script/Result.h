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
 * What one run of a script did.
 **/
struct Result final {
    std::string value;                  /**< the value of the last statement run, as a person reads it, or empty for none **/
    std::optional<Problem> problem;     /**< why it stopped early, and where when that is known **/
};

};  // namespace v3d::script
