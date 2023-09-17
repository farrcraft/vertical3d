/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context.h"

namespace v3d::event {

    Context::Context(const std::string& name) :
        name_(name),
        active_(false) {
    }

    void Context::active(bool state) {
        active_ = state;
    }

    std::string_view Context::name() const {
        return name_;
    }

    bool Context::active() const {
        return active_;
    }

};  // namespace v3d::event
