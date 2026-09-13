/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Window.h"

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

/**
 **/
Window::Window(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept :
    window_(nullptr),
    width_(300),
    height_(200),
    vulkanLoaded_(false),
    created_(false),
    logger_(logger) {
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

    // the loader has to be up before a window can be created with the vulkan flag
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        throw std::runtime_error(SDL_GetError());
    }
    vulkanLoaded_ = true;

    logger_->get()->info("Creating window {} x {}", width_, height_);
    window_ = SDL_CreateWindow("Vertical3D", width_, height_, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        logger_->get()->error("Window could not be created! SDL_Error: {}", SDL_GetError());
        return false;
    }

    // the extensions SDL needs to be able to present to this window
    uint32_t extensionCount = 0;
    const char* const* extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (extensionNames == nullptr) {
        throw std::runtime_error("Failed to get Vulkan extensions from SDL: " + std::string(SDL_GetError()));
    }
    const std::vector<const char*> extensions(extensionNames, extensionNames + extensionCount);

    instance_ = boost::make_shared<vulkan::device::Instance>(logger_, extensions);
    surface_ = boost::make_shared<vulkan::device::Surface>(instance_, window_);

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
    // the surface has to go before both the instance it belongs to and the window it presents to
    surface_.reset();
    instance_.reset();
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    if (vulkanLoaded_) {
        SDL_Vulkan_UnloadLibrary();
        vulkanLoaded_ = false;
    }
    created_ = false;
}

/**
 **/
SDL_Window* Window::sdl() noexcept {
    return window_;
}

/**
 **/
boost::shared_ptr<vulkan::device::Instance> Window::instance() const {
    return instance_;
}

/**
 **/
boost::shared_ptr<vulkan::device::Surface> Window::surface() const {
    return surface_;
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
void Window::resize(int width, int height) noexcept {
    width_ = width;
    height_ = height;
}

/**
 **/
void Window::request(int width, int height) {
    if (window_ == nullptr || width <= 0 || height <= 0) {
        return;
    }
    SDL_SetWindowSize(window_, width, height);
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

};  // namespace v3d::render::realtime
