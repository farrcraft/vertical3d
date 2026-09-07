/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "Handle.h"

namespace v3d::render::realtime {

/**
 * Where a draw item falls in the order the engine records items in.
 *
 * The fields are ordered from the coarsest grouping to the finest and packed into one
 * integer, so a whole pass sorts on a single comparison. Layer comes first because 2D
 * content is painter ordered: a sprite drawn later has to stay on top of the one under
 * it whatever pipeline or material either of them uses. Within a layer, grouping by
 * pipeline and then material is what lets the recorder merge adjacent items.
 *
 * Nothing sorts yet - the recorder walks each pass in submission order, per ADR-0004 -
 * so filling the key in is a caller's obligation that nothing enforces.
 **/
struct SortKey final {
    /**
     **/
    SortKey() noexcept;

    /**
     * @return the fields packed coarsest first, so that a < comparison orders items
     **/
    uint64_t packed() const noexcept;

    /**
     **/
    bool operator<(const SortKey& other) const noexcept;

    uint16_t layer;     /**< painter order - lower layers are recorded first **/
    uint16_t pipeline;  /**< the pipeline handle's slot, so items sharing a pipeline group **/
    uint16_t material;  /**< the material handle's slot, so items sharing a descriptor set group **/
    uint16_t depth;     /**< view depth quantized to 16 bits, for front to back ordering within a material **/
};

/**
 * A description of one draw, submitted to a pass for the engine to record.
 *
 * An item is data rather than code - the engine owns sorting, merging and recording,
 * so nothing here touches a command buffer directly. The one exception is record, the
 * escape hatch for work the model cannot yet describe; it is meant for the odd case,
 * and reaching for it routinely is the signal that the model needs extending.
 **/
struct DrawItem final {
    /**
     * How many bytes of push constants an item can carry.
     *
     * 128 is what vulkan guarantees, and taking all of it is what lets an item carry a
     * transform alongside the handful of floats a lit or graded material wants - a mat4
     * and a vec4 and a scalar is 84, and the quad primitive's lone mat4 is 64. The cost is
     * real and is paid per item per frame: an item is copied into a pass's queue by value,
     * so the unfilled part of the block is memcpyd whether or not a pipeline declared it.
     **/
    static const std::size_t pushCapacity = 128;

    /**
     **/
    DrawItem() noexcept;

    SortKey key;               /**< where the item falls in the recording order **/
    PipelineHandle pipeline;   /**< the pipeline the item draws with **/
    MaterialHandle material;   /**< the descriptor set bound at set 1 **/

    VkBuffer vertexBuffer;     /**< the geometry the draw reads, or null for a shader that needs none **/
    VkDeviceSize vertexBufferOffset;  /**< where in that buffer this item's vertices start **/
    VkBuffer indexBuffer;      /**< the indices, when the draw is indexed **/
    VkDeviceSize indexBufferOffset;   /**< where in that buffer this item's indices start **/
    VkIndexType indexType;     /**< how wide those indices are **/

    std::array<unsigned char, pushCapacity> push;  /**< the push constant block, copied so nothing outlives the item **/
    uint32_t pushSize;         /**< how much of it the pipeline's layout declared **/

    uint32_t vertices;         /**< how many vertices to draw, when the item is not indexed **/
    uint32_t firstVertex;      /**< the first vertex, or the value added to each index when it is **/
    uint32_t indices;          /**< how many indices to draw - zero for a non indexed draw **/
    uint32_t firstIndex;       /**< the first index **/
    uint32_t instances;        /**< how many instances to draw **/
    uint32_t firstInstance;    /**< the first instance **/

    bool scissored;            /**< whether the draw is cut down, rather than covering the pass **/
    VkRect2D scissor;          /**< what it is cut to, in the pixels of the image drawn into - ADR-0037 **/

    /**
     * Records the item itself, for work the fields above cannot describe.
     * The engine calls it in place of issuing its own draw.
     **/
    std::function<void(VkCommandBuffer)> record;
};

};  // namespace v3d::render::realtime
