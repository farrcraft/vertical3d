/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * One descriptor set holding an array of combined image samplers, which a draw indexes by slot.
 *
 * Every two-dimensional texture a context holds is written into a slot of its own. A shader
 * declares the array as `sampler2D textures[]` at the set number the pipeline gives this
 * layout, and reads a texture as `textures[slot]`. The set is the same for every draw, so it is
 * bound once for as long as the pipeline stays bound.
 *
 * **A slot is written only while no frame can read it.** The binding is partially bound and may
 * be updated while a frame that binds the set is pending, as long as the descriptors written are
 * not ones that frame reads. A slot is taken from the free list, which holds only slots no frame
 * in flight can name, and a released slot goes back on it through the Ring. A slot no texture
 * holds is never read, so what it names does not matter, even an image since destroyed.
 *
 * The array's size is fixed when the set is made, at the device's limits or a few thousand,
 * whichever is fewer.
 **/
class TextureArray final {
 public:
    /**
     * @throw std::runtime_error if the layout, the pool or the set cannot be created
     **/
    TextureArray(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring);

    /**
     * Destroys the pool, and the set with it, and the layout, once no frame queued or in flight
     * can still bind the set.
     **/
    ~TextureArray();

    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    /**
     * @return the layout, for a pipeline to declare
     **/
    VkDescriptorSetLayout layout() const noexcept;

    /**
     * @return the one set, to bind
     **/
    VkDescriptorSet set() const noexcept;

    /**
     * @return how many slots the array has
     **/
    uint32_t capacity() const noexcept;

    /**
     * Write an image into a free slot.
     *
     * @return the slot
     * @throw std::runtime_error if every slot is taken
     **/
    uint32_t add(const VkDescriptorImageInfo& image);

    /**
     * Give a slot back. A frame queued before now may still read it, so it is freed once the
     * frame being recorded has finished.
     **/
    void release(uint32_t slot);

    /**
     * @return how many slots are taken, counting released ones a frame may still read
     **/
    uint32_t taken() const noexcept;

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<frame::Ring> ring_;
    VkDescriptorSetLayout layout_;
    VkDescriptorPool pool_;
    VkDescriptorSet set_;
    uint32_t capacity_;
    /**
     * The slots no frame can read, the lowest last. A release in flight shares it, so it
     * outlives the array until the last has run.
     **/
    boost::shared_ptr<std::vector<uint32_t>> free_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline
