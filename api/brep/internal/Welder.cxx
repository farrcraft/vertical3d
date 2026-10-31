/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Welder.h"

#include <api/brep/Builder.h>
#include <api/brep/Index.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::brep::internal {

Welder::Welder(const Polygons& polygons, std::vector<glm::vec3>* positions)
    : polygons_(polygons), positions_(positions), vertexOf_(polygons.points.size(), INVALID_ID) {
    vertexAt_.reserve(polygons.points.size());
}

Index Welder::vertex(std::uint32_t point) {
    if (vertexOf_[point] == INVALID_ID) {
        const auto [found, added] = vertexAt_.emplace(positionKey(polygons_.points[point]),
            static_cast<Index>(positions_->size()));
        if (added) {
            positions_->push_back(polygons_.points[point]);
        }
        vertexOf_[point] = found->second;
    }
    return vertexOf_[point];
}

std::size_t Welder::PositionHash::operator()(const PositionKey& key) const noexcept {
    const std::uint64_t folded = (static_cast<std::uint64_t>(key[0]) << 32U) ^ (static_cast<std::uint64_t>(key[1]) << 16U) ^ key[2];
    return std::hash<std::uint64_t>{}(folded);
}

Welder::PositionKey Welder::positionKey(const glm::vec3& point) {
    // adding zero turns -0 into 0, which compares equal to it but has other bits
    return { std::bit_cast<std::uint32_t>(point.x + 0.0F), std::bit_cast<std::uint32_t>(point.y + 0.0F),
        std::bit_cast<std::uint32_t>(point.z + 0.0F) };
}

};  // namespace v3d::brep::internal
