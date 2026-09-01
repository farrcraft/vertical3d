/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>

#include "Handle.h"

namespace v3d::render::realtime {

    /**
     * Where a draw item falls in the order the engine records items in.
     *
     * The fields are ordered from the coarsest grouping to the finest, and packed into one
     * integer so a whole pass sorts on a single comparison. Layer comes first because 2D
     * content is painter ordered: a sprite drawn later has to stay on top of the one under
     * it whatever pipeline or material either of them uses. Within a layer, grouping by
     * pipeline and then material is what lets the recorder merge adjacent items.
     *
     * Nothing sorts yet - the recorder walks each pass in submission order, per ADR-0004 -
     * but the field has to be filled in from the first version, because auditing every call
     * site later is the expensive way to discover a sprite is behind the thing it should be
     * in front of.
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
         **/
        DrawItem() noexcept;

        SortKey key;               /**< where the item falls in the recording order **/
        PipelineHandle pipeline;   /**< the pipeline the item draws with **/
        MaterialHandle material;   /**< the descriptor set bound at set 1 **/

        uint32_t vertices;         /**< how many vertices to draw, when the item is not indexed **/
        uint32_t firstVertex;      /**< the first vertex, or the value added to each index when it is **/
        uint32_t indices;          /**< how many indices to draw - zero for a non indexed draw **/
        uint32_t firstIndex;       /**< the first index **/
        uint32_t instances;        /**< how many instances to draw **/
        uint32_t firstInstance;    /**< the first instance **/

        /**
         * Records the item itself, for work the fields above cannot describe.
         * The engine calls it in place of issuing its own draw.
         **/
        std::function<void(VkCommandBuffer)> record;
    };

};  // namespace v3d::render::realtime
