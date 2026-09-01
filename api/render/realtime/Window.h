/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <string>

#include "vulkan/Instance.h"
#include "vulkan/Surface.h"

#include "../../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     * The window everything is drawn into, and the vulkan instance and surface that reach it.
     *
     * Every window is a vulkan window: an app presents through a swapchain whether it draws
     * in two dimensions or three, per ADR-0001. What separates a 2D app from a 3D one is what
     * its passes ask for - an orthographic camera and no depth - not the window under them.
     **/
    class Window final {
     public:
        /**
         * @param logger
         **/
        explicit Window(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept;

        /**
         * Create the window, the vulkan instance, and the surface that presents to it.
         *
         * @throw std::runtime_error if the vulkan loader or SDL's extension list is missing
         * @return whether the window itself could be created
         **/
        bool create(int width, int height);

        /**
         * @return whether create() has succeeded and destroy() has not been called since
         **/
        bool created() const noexcept;

        /**
         **/
        void destroy();

        /**
         **/
        SDL_Window* sdl() noexcept;

        /**
         * @return the vulkan instance the window was created against
         **/
        boost::shared_ptr<vulkan::Instance> instance() const;

        /**
         * @return the vulkan surface the window presents to
         **/
        boost::shared_ptr<vulkan::Surface> surface() const;

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
        boost::shared_ptr<vulkan::Instance> instance_;
        boost::shared_ptr<vulkan::Surface> surface_;
        std::string caption_;
        int width_;
        int height_;
        bool vulkanLoaded_;
        bool created_;
        boost::shared_ptr<v3d::log::Logger> logger_;
    };

};  // namespace v3d::render::realtime
