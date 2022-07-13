/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Window.h"

#include <iostream>

namespace odyssey::ui {

    /**
     **/
    Window::Window(const boost::shared_ptr<odyssey::engine::Logger>& logger) noexcept :
        window_(nullptr),
        surface_(nullptr),
        logger_(logger),
        width_(300),
        height_(200) {
    }

    /**
    **/
    bool Window::create(int width, int height) {
        // Create window
        window_ = SDL_CreateWindow("Odyssey", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
        if (window_ == nullptr) {
            LOG_ERROR(logger_) << "Window could not be created! SDL_Error: " << SDL_GetError();
            return false;
        }
        // The surface contained by the window
        surface_ = SDL_GetWindowSurface(window_);

        // how do we keep these up to date when the window is resized?
        width_ = width;
        height_ = height;

        return true;
    }

    /**
     **/
    void Window::destroy() {
        SDL_FreeSurface(surface_);
        surface_ = nullptr;
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    /**
     **/
    SDL_Window* Window::sdl() noexcept {
        return window_;
    }

    int Window::width() const noexcept {
        return width_;
    }

    /**
     **/
    int Window::height() const noexcept {
        return height_;
    }

    /**
     **/
    void Window::resize(int width, int height) noexcept {
        width_ = width;
        height_ = height;
    }

    /**
     **/
    void Window::paint(SDL_Surface* surface) {
        SDL_BlitSurface(surface, nullptr, surface_, nullptr);
        SDL_UpdateWindowSurface(window_);
    }

};  // namespace odyssey::ui
