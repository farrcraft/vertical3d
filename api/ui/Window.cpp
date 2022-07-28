/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Window.h"

#include <iostream>

namespace v3d::ui {

    /**
     **/
    Window::Window(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept :
        window_(nullptr),
        surface_(nullptr),
        logger_(logger),
        width_(300),
        height_(200) {
    }

    /**
    **/
    bool Window::create(int width, int height) {
        if (width > 0) {
            width_ = width;
        }
        if (height > 0) {
            height_ = height;
        }
        LOG_INFO(logger_) << "Creating window [" << width_ << "] x [" << height_ << "]";
        window_ = SDL_CreateWindow("Odyssey", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width_, height_, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
        if (window_ == nullptr) {
            LOG_ERROR(logger_) << "Window could not be created! SDL_Error: " << SDL_GetError();
            return false;
        }
        surface_ = SDL_GetWindowSurface(window_);
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


    void Window::caption(const std::string_view& cap) {
        caption_ = cap;
        SDL_SetWindowTitle(window_, caption_.c_str());
    }

    void Window::cursor(bool state) {
        int toggle = state ? 1 : 0;
        SDL_ShowCursor(toggle);
    }

    void Window::warpCursor(int x, int y) {
        SDL_WarpMouseInWindow(window_, x, y);
    }
};  // namespace v3d::ui
