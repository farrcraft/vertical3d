/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/accel/internal/Part.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::accel {

/**
 * The hierarchies of a scene's prototypes, kept up to date as the scene changes. It keeps the
 * version of the scene it last built from, and asks the scene for what changed since.
 *
 * An update gathers the prototypes again through a geometry::Cache, which places each geometry
 * whose object did not change as it was made. A geometry's hierarchy is built again only when its
 * geometry was made again: when its object changed, or a subdivision surface was cut again. A
 * group's hierarchy, and the world's, is built again when the group or one of its instances
 * changed, its instances are not the ones it held, or one of their prototypes was built again.
 * Every other hierarchy is placed as it was built. The assembly is the one assemble() builds from
 * the same prototypes.
 **/
class Assembler final {
 public:
    Assembler(device::CpuDevice & device, const boost::shared_ptr<log::Logger> & logger);

    /** Brings the hierarchies up to the scene's version. The first update builds all of them. **/
    void update(const scene::Scene & scene);

    /** The prototypes the last update gathered. **/
    const geometry::Prototypes & prototypes() const {
        return prototypes_;
    }

    /** The hierarchies of the last update. **/
    const Assembly & assembly() const {
        return assembly_;
    }

    /** The scene's version the last update built from. **/
    std::uint64_t version() const {
        return version_;
    }

    /** The scene's index of each geometry whose hierarchy the last update built, in order. **/
    const std::vector<std::uint32_t> & builtGeometries() const {
        return builtGeometries_;
    }

    /** The scene's index of each group whose hierarchy the last update built, or kWorld. **/
    const std::vector<std::uint32_t> & builtGroups() const {
        return builtGroups_;
    }

    /** The scene's indices of the materials the last update compiled, not kept from before. **/
    const std::set<std::uint32_t> & compiledMaterials() const {
        return cache_.compiled();
    }

 private:
    /** A group's hierarchy, and the scene's indices of the instances it was built over. **/
    struct Held {
        internal::Part part;
        std::vector<std::uint32_t> members;
    };

    device::CpuDevice & device_;
    boost::shared_ptr<log::Logger> logger_;
    geometry::Cache cache_;
    geometry::Prototypes prototypes_;
    Assembly assembly_;
    std::uint64_t version_ { 0 };
    bool built_ { false };

    /** The hierarchy of each geometry, and of each group and the world, by the scene's index. **/
    std::map<std::uint32_t, internal::Part> geometryParts_;
    std::map<std::uint32_t, Held> groupParts_;

    std::vector<std::uint32_t> builtGeometries_;
    std::vector<std::uint32_t> builtGroups_;
};

};  // namespace v3d::moya::accel
