/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Context {
     public:
        /**
         **/
        Context() = default;

        /**
         * Virtual so that a Context can be narrowed back to the concrete context an
         * operation needs, and so deleting through a base pointer is well defined.
         **/
        virtual ~Context() = default;
    };
};  // namespace v3d::render::realtime
