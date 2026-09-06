/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context.h"

#include <string>

namespace v3d::event {

Context::Context(const std::string& name) :
    name_(name) {
}

std::string_view Context::name() const {
    return name_;
}

};  // namespace v3d::event
