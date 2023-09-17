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
    class MouseButton : public Event {
     public:
        /**
         **/
        MouseButton(unsigned int button, const boost::shared_ptr<Context>& context, bool pressed) noexcept;

        /**
         **/
        unsigned int button() const noexcept;

        /**
         **/
        bool pressed() const noexcept;

     private:
        unsigned int button_;
        bool pressed_;
    };
};  // namespace v3d::event
