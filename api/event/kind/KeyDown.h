/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Key.h"

#include <string>

namespace v3d::event::kind {

/**
 **/
class KeyDown final : public Key {
 public:
    /**
     **/
    KeyDown(const std::string& name, const boost::shared_ptr<Context>& context);
};
};  // namespace v3d::event::kind
