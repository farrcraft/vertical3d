/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Refused.h"

#include <stdexcept>
#include <string>

namespace v3d::script {

/**
 **/
Refused::Refused(const std::string& message) :
    std::runtime_error(message) {
}

};  // namespace v3d::script
