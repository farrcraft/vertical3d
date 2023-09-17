/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Context.h"
#include "Window2D.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Context2D : public Context {
     public:
        /**
         **/
         explicit Context2D(boost::shared_ptr<Window2D> window);

        /**
         **/
        ~Context2D();

        /**
         **/
        SDL_Renderer* handle();

     private:
        SDL_Renderer* renderer_;
    };
};  // namespace v3d::render::realtime
