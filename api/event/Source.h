/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Event.h"

#include <string>

namespace v3d::event {

    /**
     **/
    class Source final : public Event {
     public:
        /**
         **/
        Source(const std::string& name, const std::string& scope, const std::string& context);
    };
};  // namespace v3d::event
