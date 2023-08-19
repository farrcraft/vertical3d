/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Window.h"

#include <SDL.h>

namespace v3d::render::realtime {
    /**
    **/
    class Window2D final : public Window {
     public:
        /**
         * @return bool
         **/
        bool create(int width, int height, int logicalWidth, int logicalHeight);

        /**
         * @return void
         **/
        void destroy();

        /**
         * Paint a surface onto the window.
         * Typically this isn't used and texture rendering is done instead.
         **/
        void paint(SDL_Surface *surface);

        /**
         **/
        int logicalWidth() const noexcept;

        /**
         **/
        int logicalHeight() const noexcept;

     private:
        SDL_Surface* surface_;
        int logicalWidth_;
        int logicalHeight_;
    };

};  // namespace v3d::ui
