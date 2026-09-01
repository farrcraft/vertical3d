/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "Device.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

    /**
     * The pool the renderer's command buffers are allocated from.
     *
     * The buffers are reset individually rather than by resetting the whole pool, because a
     * frame in flight still owns its buffer while a later frame is being recorded.
     **/
    class CommandPool final {
     public:
        /**
         * @param device the device the pool allocates on
         * @param family the queue family the buffers will be submitted to
         **/
        CommandPool(const boost::shared_ptr<Device>& device, uint32_t family);

        /**
         **/
        ~CommandPool();

        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

        /**
         * @return the underlying pool handle
         **/
        VkCommandPool handle() const noexcept;

        /**
         * Allocate primary command buffers. They are freed with the pool, not individually.
         * @param count how many to allocate
         * @throw std::runtime_error if the allocation fails
         **/
        std::vector<VkCommandBuffer> allocate(uint32_t count) const;

     private:
        boost::shared_ptr<Device> device_;
        VkCommandPool pool_;
    };

};  // namespace v3d::render::realtime::vulkan
