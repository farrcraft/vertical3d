/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Window3D.h"
#include "vulkan/Device.h"
#include "vulkan/Swapchain.h"

#include "../../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     * The 3D render context - owns the vulkan device the window is drawn with.
     **/
    class Context3D : public Context {
     public:
        /**
         * @param logger
         * @param window the window the context renders to
         **/
        Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window3D>& window);

        /**
         **/
        ~Context3D();

        /**
         * @return the device backing the context
         **/
        boost::shared_ptr<vulkan::Device> device() const;

        /**
         * @return the chain of images being presented to the window
         **/
        boost::shared_ptr<vulkan::Swapchain> swapchain() const;

        /**
         * Rebuild the swapchain against the window's current size.
         * Call this when presenting reports the chain has gone out of date.
         **/
        void resize();

     private:
        boost::shared_ptr<Window3D> window_;
        boost::shared_ptr<vulkan::Device> device_;
        boost::shared_ptr<vulkan::Swapchain> swapchain_;
    };
};  // namespace v3d::render::realtime
