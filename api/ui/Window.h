/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../log/Logger.h"

#include <SDL.h>

namespace v3d::ui {
    /**
    **/
    class Window final {
     public:
        /**
         * @param Logger* logger
         **/
        Window(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept;

        /**
         * @return bool
         **/
        bool create(int width, int height);

        /**
         * @return void
         **/
        void destroy();

        /**
         **/
        SDL_Window* sdl() noexcept;

        /**
         * Paint a surface onto the window.
         * Typically this isn't used and texture rendering is done instead.
         **/
        void paint(SDL_Surface *surface);

        /**
         **/
        void resize(int width, int height) noexcept;

        /**
         **/
        int width() const noexcept;

        /**
         **/
        int height() const noexcept;

        /**
         * Set the window caption.
         * @param cap the new window caption
         */
        void caption(const std::string_view& cap);

        /**
         * Toggle mouse cursor visibility
         * @param state whether to enable or disable
         */
        void cursor(bool state);

        /**
         * Move the mouse cursor to a new position in the window
         */
        void warpCursor(int x, int y);

     private:
        SDL_Window* window_;
        SDL_Surface* surface_;
        std::string caption_;
        int width_;
        int height_;
        boost::shared_ptr<v3d::log::Logger> logger_;
    };

};  // namespace v3d::ui
