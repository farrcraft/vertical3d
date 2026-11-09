/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/internal/Piece.h>

#include <cstdint>
#include <map>
#include <set>

namespace v3d::moya::geometry {

namespace internal {
class Walk;
};  // namespace internal

/**
 * What gather() made of each geometry, kept by its owner from one gather to the next. A gather
 * places a geometry the cache holds as it was made, and makes only the others. The owner forgets
 * each geometry whose object changed before it gathers again.
 *
 * A subdivision surface is placed from the cache only when it is measured from the same dicing
 * camera, at the same edge length and placement, as it was cut for. Otherwise it is cut again from
 * the limit surface the cache holds. A gather drops every geometry it did not place.
 **/
class Cache final {
 public:
    /** Forgets the geometry at an index of the scene, so that the next gather makes it again. **/
    void forget(std::uint32_t index) {
        pieces_.erase(index);
    }

    /** The scene's indices of the geometries the last gather made, not placed from the cache. **/
    const std::set<std::uint32_t> & made() const {
        return made_;
    }

 private:
    friend class internal::Walk;

    std::map<std::uint32_t, internal::Piece> pieces_;
    std::set<std::uint32_t> made_;
};

};  // namespace v3d::moya::geometry
