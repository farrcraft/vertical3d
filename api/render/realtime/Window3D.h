/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Window.h"

#include <SDL.h>

namespace v3d::render::realtime {
    /**
    **/
    class Window3D final : public Window {
    public:
        Window3D(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept;

        /**
         * @return bool
         **/
        bool create(int width, int height);

        /**
         * @return void
         **/
        void destroy();

    private:
        SDL_GLContext context_;  // Part of window for now - could move to Context3D later...
    };

};  // namespace v3d::ui
