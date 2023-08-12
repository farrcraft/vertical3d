/**
 * Vertical3D
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../ui/Window.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Context {
     public:
        /**
         **/
        Context(boost::shared_ptr<v3d::ui::Window> window);

        /**
         **/
        ~Context();

        /**
         **/
        SDL_Renderer* handle();

     private:
        SDL_Renderer* renderer_;
    };
};  // namespace v3d::render::realtime
