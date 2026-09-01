/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Window3D.h"
#include "vulkan/Device.h"
#include "vulkan/PipelineCache.h"
#include "vulkan/Presenter.h"
#include "vulkan/Resources.h"
#include "vulkan/Swapchain.h"

#include "../../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     * The 3D render context - owns the vulkan device the window is drawn with, the chain of
     * images presented to it, and the per frame machinery that gets one onto the screen.
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
         * @return the acquire, submit and present loop the frames go through
         **/
        boost::shared_ptr<vulkan::Presenter> presenter() const;

        /**
         * @return the cache every pipeline built on this device is compiled against
         **/
        boost::shared_ptr<vulkan::PipelineCache> pipelineCache() const;

        /**
         * @return the pipelines, materials and textures a draw item can name by handle
         **/
        boost::shared_ptr<vulkan::Resources> resources() const;

        /**
         * Rebuild the swapchain against the window's current size, and everything that is
         * sized by it. Call this when presenting reports the chain has gone out of date.
         **/
        void resize();

     private:
        boost::shared_ptr<Window3D> window_;
        boost::shared_ptr<vulkan::Device> device_;
        boost::shared_ptr<vulkan::Swapchain> swapchain_;
        boost::shared_ptr<vulkan::PipelineCache> pipelineCache_;
        boost::shared_ptr<vulkan::Resources> resources_;
        // last, so that it is torn down first - nothing else may go away while a frame it
        // submitted is still in flight
        boost::shared_ptr<vulkan::Presenter> presenter_;
    };
};  // namespace v3d::render::realtime
