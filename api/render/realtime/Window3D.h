/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Window.h"
#include "vulkan/Instance.h"
#include "vulkan/Surface.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <boost/shared_ptr.hpp>

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
         * @return bool
         **/
        bool created() const;

        /**
         * @return void
         **/
        void destroy();

        /**
         * @return the vulkan instance the window was created against
         **/
        boost::shared_ptr<vulkan::Instance> instance() const;

        /**
         * @return the vulkan surface the window presents to
         **/
        boost::shared_ptr<vulkan::Surface> surface() const;

     private:
        boost::shared_ptr<vulkan::Instance> instance_;
        boost::shared_ptr<vulkan::Surface> surface_;
        SDL_GLContext context_;  // Part of window for now - could move to Context3D later...
        bool vulkanLoaded_;
        bool created_;
    };

};  // namespace v3d::render::realtime
