/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Window3D.h"

#include <iostream>

namespace v3d::render::realtime {
    Window3D::Window3D(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept : Window(logger) {
    }

    /**
    **/
    bool Window3D::create(int width, int height) {
        if (!Window::create(width, height, true)) {
            return false;
        }

        // use OpenGL >= 3.2
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

        // 24 bit back buffer
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        context_ = SDL_GL_CreateContext(sdl());

        return true;
    }

    /**
     **/
    void Window3D::destroy() {
        if (context_) {
            SDL_GL_DeleteContext(context_);
            context_ = nullptr;
        }
        Window::destroy();
    }

};  // namespace v3d::ui
