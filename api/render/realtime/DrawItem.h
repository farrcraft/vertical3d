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
#include "SortKey.h"

namespace v3d::render::realtime {

/**
 * A description of one draw, submitted to a pass for the engine to record.
 *
 * An item is data rather than code. The engine sorts, merges and records items, so nothing
 * here writes to a command buffer directly. The exception is record, an escape hatch for
 * work the fields cannot describe yet. It is meant for rare cases; needing it routinely
 * means the item needs a new field.
 **/
struct DrawItem final {
    /**
     * How many bytes of push constants an item can carry.
     *
     * 128 is the minimum Vulkan guarantees. Using all of it lets an item carry a transform
     * alongside the few values a lit or graded material needs. The lit renderer's mat4,
     * vec4, scalar and joint index take 88 bytes, and the quad primitive's mat4 and text flag
     * take 68. The cost is paid per item per frame: an item is copied into a pass's queue by
     * value, so the unused part of the block is copied whether or not a pipeline declared it.
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
    VkRect2D scissor;          /**< what it is cut to, in the pixels of the image drawn into **/

    /**
     * Records the item itself, for work the fields above cannot describe.
     * The engine calls it in place of issuing its own draw.
     **/
    std::function<void(VkCommandBuffer)> record;
};

};  // namespace v3d::render::realtime
