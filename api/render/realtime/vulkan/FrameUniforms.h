/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Buffer.h"
#include "Device.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::vulkan {

/**
 * Set 0, the per frame frequency of ADR-0008 - the camera a whole pass draws through.
 *
 * A scene of a few hundred draws shares one view and one projection, so they are bound
 * once for the pass rather than pushed per draw. This owns the layout every pipeline in
 * the engine declares at set 0, which is what makes those pipelines interchangeable
 * within a pass: a descriptor set bound for one stays bound across a pipeline change to
 * another built against the same layout.
 *
 * There is a slot per pass per frame in flight, because a frame's several passes have
 * different cameras and the device may still be reading the set two frames back. Slots
 * are added as passes demand them and never given back - a frame's pass count settles in
 * the first few frames.
 *
 * A slot is written during recording, which is after the presenter has waited on the
 * frame's fence, so nothing is reading what is about to be overwritten.
 **/
class FrameUniforms final {
 public:
    /**
     * What set 0 holds, laid out to match the uniform block the shaders declare.
     * std140 puts a mat4 and a vec4 at their natural offsets, so the struct maps
     * straight across with no padding of our own.
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
     * @param framesInFlight how many frames may be recorded ahead of the device
     * @throw std::runtime_error if the layout cannot be created
     **/
    FrameUniforms(const boost::shared_ptr<Device>& device, uint32_t framesInFlight);

    /**
     **/
    ~FrameUniforms();

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

        boost::shared_ptr<Buffer> buffer;
        VkDescriptorSet set;
    };

    /**
     **/
    void createLayout();

    /**
     * Add a descriptor pool, because the last one is full or there is none.
     **/
    void addPool();

    /**
     * Allocate one more slot for a frame, buffer and descriptor set together.
     **/
    Slot addSlot();

    boost::shared_ptr<Device> device_;
    VkDescriptorSetLayout layout_;
    std::vector<VkDescriptorPool> pools_;
    uint32_t remaining_;                     /**< sets left in the last pool **/
    std::vector<std::vector<Slot>> slots_;   /**< a ring of slots per frame in flight **/
    uint32_t frame_;                         /**< which ring write() is filling **/
    std::size_t cursor_;                     /**< how far into that ring it has got **/
};

};  // namespace v3d::render::realtime::vulkan
