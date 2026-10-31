/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Builder.h>
#include <api/brep/Index.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::brep::internal {

/**
 * Gives each position one vertex. Only the points a face uses become vertices.
 **/
class Welder final {
 public:
    Welder(const Polygons& polygons, std::vector<glm::vec3>* positions);

    Index vertex(std::uint32_t point);

 private:
    /**
     * A position's exact bits, for finding every point at it in one lookup.
     **/
    using PositionKey = std::array<std::uint32_t, 3>;

    /**
     * A float holding a small whole number has its low bits clear, and a table picks a bucket by
     * the low bits of the hash. The words are therefore folded together and handed to the
     * standard hash, which mixes every bit.
     **/
    struct PositionHash {
        std::size_t operator()(const PositionKey& key) const noexcept;
    };

    static PositionKey positionKey(const glm::vec3& point);

    const Polygons& polygons_;
    std::vector<glm::vec3>* positions_;
    std::vector<Index> vertexOf_;
    std::unordered_map<PositionKey, Index, PositionHash> vertexAt_;
};

};  // namespace v3d::brep::internal
