/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * Descriptor sets of one layout, and the layout itself.
 *
 * Sets are allocated out of a list of Vulkan pools. The list grows by a fixed count whenever
 * the last one is full, so the number of sets a frame or a scene needs is not fixed up front.
 * Individual sets are not freed: one handed back is kept and given out again once the ring
 * reports that no frame in flight can still be binding it.
 *
 * Every set a pool hands out has the same layout, so a set allocated here can be bound by
 * any pipeline that declared that layout at the same set number.
 **/
class DescriptorPool final {
 public:
    /**
     * @param device the device the layout, the pools and the sets are created on
     * @param ring the frames in flight, which hold back a released set until they finish
     * @param bindings the layout's bindings, which also decide how much each pool holds
     * @param setsPerPool how many sets a pool is created with, and so how often one is added
     * @param name what the sets are for, for the messages an error throws
     * @throw std::runtime_error if the layout cannot be created
     **/
    DescriptorPool(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t setsPerPool, const std::string& name);

    /**
     * Destroys the pools, which frees every set allocated from them, and then the layout. Both
     * are destroyed through the ring, once no frame queued or in flight can still bind a set.
     **/
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    /**
     * @return the layout every set from here has, for a pipeline to declare
     **/
    VkDescriptorSetLayout layout() const noexcept;

    /**
     * A set to write and bind: one that was released, if any has come back, or a new one.
     * Its contents are whatever was last written into it, so the caller writes it before
     * binding it.
     *
     * @throw std::runtime_error if a pool cannot be added or a set cannot be allocated
     **/
    VkDescriptorSet allocate();

    /**
     * Hand a set back, to be given out again once no frame in flight can be binding it.
     **/
    void release(VkDescriptorSet set);

    /**
     * @return how many vulkan pools have been created
     **/
    std::size_t pools() const noexcept;

 private:
    /**
     * Add a vulkan pool, because the last one is full or there is none.
     **/
    void addPool();

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<frame::Ring> ring_;
    std::string name_;
    uint32_t setsPerPool_;
    VkDescriptorSetLayout layout_;
    std::vector<VkDescriptorPoolSize> sizes_;  /**< what one pool holds, by descriptor type **/
    std::vector<VkDescriptorPool> pools_;
    uint32_t remaining_;                       /**< sets left in the last pool **/
    /**< sets handed back that no frame still binds. Shared with what the ring runs, because
         the ring can outlive this **/
    boost::shared_ptr<std::vector<VkDescriptorSet>> spare_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline
