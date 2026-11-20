/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/internal/Piece.h>
#include <moya/libmoya/volume/Grid.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::geometry {

namespace internal {
class Walk;
};  // namespace internal

/**
 * What gather() made of each geometry, and each grid it read, kept by its owner from one gather
 * to the next. A gather places a geometry the cache holds as it was made, and makes only the
 * others, and reads only the grids the cache does not hold. The owner forgets each geometry and
 * field whose object changed before it gathers again, so a changed field reads its grid again
 * and nothing else does.
 *
 * A subdivision surface is placed from the cache only when it is measured from the same dicing
 * camera, at the same edge length and placement, as it was cut for. Otherwise it is cut again from
 * the limit surface the cache holds. A gather drops every geometry it did not place, and every
 * grid it did not read.
 **/
class Cache final {
 public:
    /**
     * Forgets the geometry or the grid at an index of the scene, so that the next gather makes or
     * reads it again.
     **/
    void forget(std::uint32_t index) {
        pieces_.erase(index);
        grids_.erase(index);
    }

    /** The scene's indices of the geometries the last gather made, not placed from the cache. **/
    const std::set<std::uint32_t> & made() const {
        return made_;
    }

 private:
    friend class internal::Walk;

    std::map<std::uint32_t, internal::Piece> pieces_;
    std::set<std::uint32_t> made_;

    /** Each grid a field read, by the scene's index of its field. **/
    std::map<std::uint32_t, boost::shared_ptr<const volume::Grid>> grids_;

    /** The primvars shading read, by their numbers, when the pieces were made. **/
    std::vector<std::string> primvarNames_;
};

};  // namespace v3d::moya::geometry
