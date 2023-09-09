/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Event.h"

#include <string>

namespace v3d::event {

    /**
     **/
    class Source final : public Event {
     public:
        /**
         **/
        Source(const std::string& name, const boost::shared_ptr<Context>& context);
    };
};  // namespace v3d::event
