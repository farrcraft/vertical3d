/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Window2D.h"

#include <iostream>

namespace v3d::render::realtime {
    Window2D::Window2D(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept : Window(logger) {
    }

    /**
    **/
    bool Window2D::create(int width, int height, int logicalWidth, int logicalHeight) {
        if (!Window::create(width, height)) {
            return false;
        }
        if (logicalWidth > 0) {
            logicalWidth_ = logicalWidth;
        }
        else {
            logicalWidth_ = width;
        }
        if (logicalHeight > 0) {
            logicalHeight_ = logicalHeight;
        }
        else {
            logicalHeight_ = height;
        }
        surface_ = SDL_GetWindowSurface(sdl());
        return true;
    }

    /**
     **/
    void Window2D::destroy() {
        SDL_FreeSurface(surface_);
        surface_ = nullptr;
        Window::destroy();
    }

    int Window2D::logicalWidth() const noexcept {
        return logicalWidth_;
    }

    /**
     **/
    int Window2D::logicalHeight() const noexcept {
        return logicalHeight_;
    }

    void Window2D::logicalSize(int width, int height) {
        logicalWidth_ = width;
        logicalHeight_ = height;

    }

    /**
     **/
    void Window2D::paint(SDL_Surface* surface) {
        SDL_BlitSurface(surface, nullptr, surface_, nullptr);
        SDL_UpdateWindowSurface(sdl());
    }

};  // namespace v3d::ui
