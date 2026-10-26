/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Window.h"

#include <api/log/Logger.h>

#include <string>
#include <string_view>

namespace v3d::window {

namespace {

// the size a window reports before it is created
constexpr int DEFAULT_WIDTH = 300;
constexpr int DEFAULT_HEIGHT = 200;

};  // namespace

/**
 **/
Window::Window(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept :
    window_(nullptr),
    width_(DEFAULT_WIDTH),
    height_(DEFAULT_HEIGHT),
    created_(false),
    logger_(logger) {
}

/**
**/
bool Window::create(const glm::ivec2& size) {
    if (size.x > 0) {
        width_ = size.x;
    }
    if (size.y > 0) {
        height_ = size.y;
    }

    logger_->get()->info("Creating window {} x {}", width_, height_);
    window_ = SDL_CreateWindow("Vertical3D", width_, height_, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        logger_->get()->error("Window could not be created! SDL_Error: {}", SDL_GetError());
        return false;
    }
    created_ = true;
    return true;
}

/**
 **/
bool Window::created() const noexcept {
    return created_;
}

/**
 **/
void Window::destroy() {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    created_ = false;
}

/**
 **/
SDL_Window* Window::sdl() noexcept {
    return window_;
}

/**
 **/
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
void Window::resize(const glm::ivec2& size) noexcept {
    width_ = size.x;
    height_ = size.y;
}

/**
 **/
void Window::request(const glm::ivec2& size) {
    if (window_ == nullptr || size.x <= 0 || size.y <= 0) {
        return;
    }
    SDL_SetWindowSize(window_, size.x, size.y);
}

/**
 **/
void Window::caption(const std::string_view& cap) {
    caption_ = cap;
    SDL_SetWindowTitle(window_, caption_.c_str());
}

/**
 **/
bool Window::focused() const {
    if (window_ == nullptr) {
        return false;
    }
    return (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

/**
 **/
bool Window::fullscreen(bool on) {
    if (window_ == nullptr) {
        return false;
    }
    if (!SDL_SetWindowFullscreen(window_, on)) {
        logger_->get()->error("The window could not {} full screen: {}", on ? "go" : "leave", SDL_GetError());
        return false;
    }
    return true;
}

/**
 **/
bool Window::fullscreen() const {
    if (window_ == nullptr) {
        return false;
    }
    return (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
}

/**
 **/
bool Window::textInput(bool on) {
    if (window_ == nullptr) {
        return false;
    }
    const bool ok = on ? SDL_StartTextInput(window_) : SDL_StopTextInput(window_);
    if (!ok) {
        logger_->get()->warn("Text input could not be turned {}, so typing may not reach a ui: {}",
            on ? "on" : "off", SDL_GetError());
    }
    return ok;
}

/**
 **/
bool Window::textInput() const {
    return window_ != nullptr && SDL_TextInputActive(window_);
}

/**
 **/
bool Window::relativeMouse(bool on) {
    if (window_ == nullptr) {
        return false;
    }
    const bool ok = SDL_SetWindowRelativeMouseMode(window_, on);
    if (!ok) {
        logger_->get()->warn("Relative mouse mode could not be turned {}: {}", on ? "on" : "off", SDL_GetError());
    }
    return ok;
}

/**
 **/
bool Window::relativeMouse() const {
    return window_ != nullptr && SDL_GetWindowRelativeMouseMode(window_);
}

/**
 **/
void Window::cursor(bool state) {
    if (state) {
        SDL_ShowCursor();
    } else {
        SDL_HideCursor();
    }
}

/**
 **/
void Window::warpCursor(int x, int y) {
    SDL_WarpMouseInWindow(window_, static_cast<float>(x), static_cast<float>(y));
}

};  // namespace v3d::window
