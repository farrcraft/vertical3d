/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <array>
#include <string>

namespace v3d::rib::requests {

/**
 * RenderMan's RiVolume: a volume of a type, inside a box, with a grid of voxels for its varying
 * values. The type is a shape, such as `"box"` or `"ellipsoid"`, or `"blobbydso:"` and the plugin
 * that gives the volume's values, such as `"blobbydso:impl_openvdb"`. A plugin's volume gives its
 * own bounds and voxels, and RIB writes the box and the counts all the same.
 **/
struct Volume {
    std::string type{};
    /** xmin, xmax, ymin, ymax, zmin and zmax, in object space **/
    std::array<float, 6> bound { -1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F };
    /** how many voxels the varying values cover along x, y and z **/
    std::array<unsigned int, 3> voxels { 0, 0, 0 };
};

};  // namespace v3d::rib::requests
