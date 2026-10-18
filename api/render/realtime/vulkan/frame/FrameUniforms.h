/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Ring.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * Set 0: the camera a whole pass draws through.
 *
 * Every draw in a pass shares one view and one projection, so they are bound once for the
 * pass, not pushed per draw. FrameUniforms owns the layout every pipeline in the engine
 * declares at set 0, so those pipelines are interchangeable within a pass. A set bound for
 * one pipeline stays bound when the pass binds another built against the same layout.
 *
 * There is one slot per pass per frame in flight. Each pass of a frame has its own camera,
 * and the device may still read a slot written framesInFlight frames ago. Slots are added as
 * passes need them and are never released. A frame's pass count settles in the first few
 * frames.
 *
 * A slot is written during recording, after the frame's fence has been waited on, so the
 * device is not reading it.
 **/
class FrameUniforms final {
 public:
    /**
     * What set 0 holds, laid out to match the uniform block the shaders declare.
     * std140 puts a mat4 and a vec4 at their natural offsets, so the struct maps
     * straight across with no explicit padding.
     **/
    struct Camera final {
        Camera() noexcept;

        glm::mat4 view;            /**< world to view **/
        glm::mat4 projection;      /**< view to clip **/
        glm::mat4 viewProjection;  /**< the product, so a vertex shader needs one multiply **/
        glm::vec4 viewport;        /**< x, y, width, height of the region the pass draws into **/
    };

    /**
     * @param device the device the buffers and sets are allocated on
     * @param ring the frames in flight. Each has its own list of slots.
     * @throw std::runtime_error if the layout cannot be created
     **/
    FrameUniforms(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Ring>& ring);

    FrameUniforms(const FrameUniforms&) = delete;
    FrameUniforms& operator=(const FrameUniforms&) = delete;

    /**
     * @return the set 0 layout every pipeline in the engine has to declare
     **/
    VkDescriptorSetLayout layout() const noexcept;

    /**
     * Start writing into a frame's slots, from the first one.
     * @param frame which frame in flight is being recorded
     **/
    void begin(uint32_t frame) noexcept;

    /**
     * Write a pass's camera into the next slot of the frame begin() named.
     * @return the set to bind at 0 for that pass, or null if there is no such frame
     * @throw std::runtime_error if a slot cannot be allocated
     **/
    VkDescriptorSet write(const Camera& camera);

 private:
    /**
     * A uniform buffer and the set that points at it, one per pass per frame.
     **/
    struct Slot {
        Slot() noexcept;

        boost::shared_ptr<memory::Buffer> buffer;
        VkDescriptorSet set;
    };

    /**
     * Allocate one more slot for a frame, buffer and descriptor set together.
     **/
    Slot addSlot();

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::DescriptorPool> pool_;
    std::vector<std::vector<Slot>> slots_;   /**< a list of slots per frame in flight **/
    uint32_t frame_;                         /**< the frame whose slots write() fills **/
    std::size_t cursor_;                     /**< the next slot write() fills **/
};

};  // namespace v3d::render::realtime::vulkan::frame
