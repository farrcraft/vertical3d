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

namespace {

// A coordinate's bits, with -0 given the bits of 0, which it compares equal to. The sign bit is
// cleared on the integer: an optimiser may fold x + 0.0F to x, which keeps -0's sign.
std::uint32_t bits(float coordinate) {
    const std::uint32_t raw = std::bit_cast<std::uint32_t>(coordinate);
    return raw == 0x80000000U ? 0U : raw;
}

};  // namespace

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
    return { bits(point.x), bits(point.y), bits(point.z) };
}

};  // namespace v3d::brep::internal
