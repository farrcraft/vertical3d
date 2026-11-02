/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DepthOrder.h"

#include "World.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <vector>

namespace v3d::render::realtime::canvas {

/**
 **/
void DepthOrder::clear() {
    entries_.clear();
}

/**
 **/
void DepthOrder::quad(float key, const World::Corners& corners, const glm::vec4& colour) {
    // what World does with an untextured quad, so the two draw it the same way
    quad(key, corners, glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), colour, resource::TextureHandle());
}

/**
 **/
void DepthOrder::quad(float key, const World::Corners& corners, const glm::vec2& uv0,
    const glm::vec2& uv1, const glm::vec4& colour, const resource::TextureHandle& texture) {
    entries_.push_back(Entry{key, corners, uv0, uv1, colour, texture, std::nullopt, std::nullopt});
}

/**
 **/
void DepthOrder::glyph(float key, const World::Corners& corners, const World::Glyph& glyph, const glm::vec4& colour) {
    entries_.push_back(Entry{key, corners, glm::vec2(0.0F), glm::vec2(0.0F), colour, glyph.curveTexture, glyph, std::nullopt});
}

/**
 **/
void DepthOrder::field(float key, uint32_t layer, const World::Corners& corners, const glm::vec2& uv0, const glm::vec2& uv1,
    const glm::vec4& colour, const resource::TextureHandle& texture, uint32_t ramp, float contrast) {
    entries_.push_back(Entry{key, corners, uv0, uv1, colour, texture, std::nullopt, ramp, layer, contrast});
}

/**
 **/
void DepthOrder::into(World* canvas) const {
    // an order over indices rather than over the entries, so the collection is left as it was
    // added and a caller can hand it to a second canvas
    std::vector<std::size_t> order(entries_.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [this](std::size_t left, std::size_t right) {
        const Entry& first = entries_[left];
        const Entry& second = entries_[right];
        // a key that is not a number goes last, because a sort given NaN compared as a number
        // has no consistent order and its result is undefined
        const bool firstMissing = std::isnan(first.key);
        const bool secondMissing = std::isnan(second.key);
        if (firstMissing != secondMissing) {
            return secondMissing;
        }
        if (!firstMissing && first.key != second.key) {
            return first.key > second.key;
        }
        // an effect lies beneath its glyph, and a glyph's effects in their order
        if (first.ramp.has_value() != second.ramp.has_value()) {
            return first.ramp.has_value();
        }
        if (first.ramp.has_value() && first.layer != second.layer) {
            return first.layer < second.layer;
        }
        if (first.texture != second.texture) {
            return first.texture < second.texture;
        }
        const resource::TextureHandle firstBands = first.glyph ? first.glyph->bandTexture : resource::TextureHandle();
        const resource::TextureHandle secondBands = second.glyph ? second.glyph->bandTexture : resource::TextureHandle();
        return firstBands < secondBands;
    });

    for (std::size_t index : order) {
        const Entry& entry = entries_[index];
        if (entry.glyph) {
            canvas->glyph(entry.corners, *entry.glyph, entry.colour);
        } else if (entry.ramp) {
            canvas->field(entry.corners, entry.uv0, entry.uv1, entry.colour, entry.texture, *entry.ramp, entry.contrast);
        } else {
            canvas->quad(entry.corners, entry.uv0, entry.uv1, entry.colour, entry.texture);
        }
    }
}

/**
 **/
std::size_t DepthOrder::size() const noexcept {
    return entries_.size();
}

};  // namespace v3d::render::realtime::canvas
