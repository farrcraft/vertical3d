/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Key.h"

#include <string>

namespace v3d::event::kind {

/**
 **/
class KeyUp final : public Key {
 public:
    /**
     **/
    KeyUp(const std::string& name, const boost::shared_ptr<Context>& context);
};
};  // namespace v3d::event::kind
