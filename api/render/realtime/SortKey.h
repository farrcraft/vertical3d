/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

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
 * A pass is recorded in submission order unless Pass::sort() asks for key order, and
 * filling the key in is a caller's obligation that nothing enforces.
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

};  // namespace v3d::render::realtime
