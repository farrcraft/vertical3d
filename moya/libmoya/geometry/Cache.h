/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/internal/Piece.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/volume/Grid.h>

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::geometry {

namespace internal {
class Programs;
class Walk;
};  // namespace internal

/**
 * What gather() made of each geometry, and each grid it read, kept by its owner from one gather
 * to the next. A gather places a geometry the cache holds as it was made, and makes only the
 * others, and reads only the grids the cache does not hold. The owner forgets each geometry and
 * field whose object changed before it gathers again, so a changed field reads its grid again
 * and nothing else does.
 *
 * Each material's compiled programs are kept as well. A gather compiles a material again when its
 * object, a node or connection of its network, or the shader search path changed. It compiles it
 * again too when the primvars the materials before it read are not those it was compiled after.
 * A material kept is therefore the one a gather of no cache compiles.
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
        forgotten_.insert(index);
    }

    /** The scene's indices of the geometries the last gather made, not placed from the cache. **/
    const std::set<std::uint32_t> & made() const {
        return made_;
    }

    /** The scene's indices of the materials the last gather compiled, not kept from the cache. **/
    const std::set<std::uint32_t> & compiled() const {
        return compiled_;
    }

 private:
    friend class internal::Programs;
    friend class internal::Walk;

    /** What a gather compiled of one material, and what it was compiled from. **/
    struct Program {
        scene::Handle handle;
        /** The scene's index of every node and connection of the material's network. **/
        std::set<std::uint32_t> network;
        /** The primvars read before the material was compiled, and after it. **/
        std::vector<std::string> before;
        std::vector<std::string> after;
        std::optional<material::Compiled> displacement;
        std::optional<material::Compiled> surface;
        std::optional<material::Compiled> volume;
    };

    std::map<std::uint32_t, internal::Piece> pieces_;
    std::set<std::uint32_t> made_;

    /** Each grid a field read, by the scene's index of its field. **/
    std::map<std::uint32_t, boost::shared_ptr<const volume::Grid>> grids_;

    /** The primvars shading read, by their numbers, when the pieces were made. **/
    std::vector<std::string> primvarNames_;

    /** Each material's programs, by the scene's index of the material. **/
    std::map<std::uint32_t, Program> programs_;
    std::set<std::uint32_t> compiled_;

    /** The shader search path the programs were compiled under. **/
    std::string shaderSearchPath_;

    /** The scene's index of every object forgotten since the last gather. **/
    std::set<std::uint32_t> forgotten_;
};

};  // namespace v3d::moya::geometry
