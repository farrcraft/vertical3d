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
    class Key : public Event {
     public:
        /**
         **/
        Key(const std::string& name, const boost::shared_ptr<Context>& context, bool pressed) noexcept;

        /**
         **/
        bool pressed() const noexcept;

     private:
        bool pressed_;
    };
};  // namespace v3d::event
