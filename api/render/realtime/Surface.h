/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../image/Image.h"

#include <boost/shared_ptr.hpp>
#include <SDL.h>

namespace v3d::render::realtime {
    /**
     * Represents an SDL surface
     **/
    class Surface {
     public:
        /**
         * Create an SDL surface from an image object
         **/
        Surface(boost::shared_ptr<v3d::image::Image> image);

        /**
         **/
        ~Surface();

        /**
         **/
        SDL_Surface* surface();

     private:
        SDL_Surface* surface_;
    };
};  // namespace v3d::render::realtime
