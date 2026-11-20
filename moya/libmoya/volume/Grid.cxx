/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/volume/Grid.h>

#include <moya/libmoya/kernel/volume/Block.h>

// MSVC reports code NanoVDB's tools cannot reach once it inlines them, as it does NanoVDB's grid
// header, so the warning is switched off around them too.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <nanovdb/GridHandle.h>
#include <nanovdb/HostBuffer.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <openvdb/Exceptions.h>
#include <openvdb/Grid.h>
#include <openvdb/io/File.h>
#include <openvdb/math/Coord.h>
#include <openvdb/math/Vec3.h>
#include <openvdb/openvdb.h>

#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::volume {

Grid read(const std::string & path, const std::string & name) {
    // OpenVDB registers the grid types a file is read by first, once however often it is asked
    openvdb::initialize();
    openvdb::GridBase::Ptr base;
    try {
        openvdb::io::File file(path);
        file.open();
        if (!file.hasGrid(name)) {
            file.close();
            throw std::runtime_error(path + " holds no grid named " + name);
        }
        base = file.readGrid(name);
        file.close();
    } catch (const openvdb::Exception & failed) {
        throw std::runtime_error(path + " cannot be read: " + failed.what());
    }
    const openvdb::FloatGrid::Ptr floats = openvdb::gridPtrCast<openvdb::FloatGrid>(base);
    if (!floats) {
        throw std::runtime_error("grid " + name + " of " + path + " holds " + base->valueType() + " values, not floats");
    }
    const nanovdb::GridHandle<nanovdb::HostBuffer> handle = nanovdb::tools::createNanoGrid(*floats);
    Grid out;
    out.blocks.resize((handle.bufferSize() + sizeof(kernel::volume::Block) - 1U) / sizeof(kernel::volume::Block));
    std::memcpy(out.blocks.data(), handle.data(), handle.bufferSize());
    const openvdb::CoordBBox active = floats->evalActiveVoxelBoundingBox();
    if (active.empty()) {
        return out;
    }
    // a lookup reads the voxels from its floor to one past it, so it can read an active voxel
    // from a point up to one voxel beyond the active ones on either side
    const openvdb::Coord least = active.min() - openvdb::Coord(1);
    const openvdb::Coord most = active.max() + openvdb::Coord(1);
    out.lower = glm::vec3(std::numeric_limits<float>::infinity());
    out.upper = glm::vec3(-std::numeric_limits<float>::infinity());
    for (int corner = 0; corner < 8; ++corner) {
        const openvdb::Vec3d index((corner & 1) != 0 ? most.x() : least.x(), (corner & 2) != 0 ? most.y() : least.y(), (corner & 4) != 0 ? most.z() : least.z());
        const openvdb::Vec3d world = floats->indexToWorld(index);
        const glm::vec3 point { static_cast<float>(world.x()), static_cast<float>(world.y()), static_cast<float>(world.z()) };
        out.lower = glm::min(out.lower, point);
        out.upper = glm::max(out.upper, point);
    }
    return out;
}

};  // namespace v3d::moya::volume
