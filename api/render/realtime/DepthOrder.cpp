/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DepthOrder.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

namespace v3d::render::realtime {

/**
 **/
void DepthOrder::clear() {
    entries_.clear();
}

/**
 **/
void DepthOrder::quad(float key, const WorldCanvas::Corners& corners, const glm::vec4& colour) {
    // what WorldCanvas does with an untextured quad, so the two draw it the same way
    quad(key, corners, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), colour, TextureHandle());
}

/**
 **/
void DepthOrder::quad(float key, const WorldCanvas::Corners& corners, const glm::vec2& uv0,
    const glm::vec2& uv1, const glm::vec4& colour, const TextureHandle& texture) {
    entries_.push_back(Entry{key, corners, uv0, uv1, colour, texture});
}

/**
 **/
void DepthOrder::into(WorldCanvas* canvas) const {
    // an order over indices rather than over the entries, so the collection is left as it was
    // added and a caller can hand it to a second canvas
    std::vector<std::size_t> order(entries_.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [this](std::size_t left, std::size_t right) {
        const Entry& a = entries_[left];
        const Entry& b = entries_[right];
        if (a.key != b.key) {
            return a.key > b.key;
        }
        return a.texture < b.texture;
    });

    for (std::size_t index : order) {
        const Entry& entry = entries_[index];
        canvas->quad(entry.corners, entry.uv0, entry.uv1, entry.colour, entry.texture);
    }
}

/**
 **/
std::size_t DepthOrder::size() const noexcept {
    return entries_.size();
}

};  // namespace v3d::render::realtime
