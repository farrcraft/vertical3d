/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/volume/Block.h>
#include <moya/libmoya/kernel/volume/Grid.h>
#include <moya/libmoya/kernel/volume/GridPlace.h>
#include <moya/libmoya/kernel/volume/Nano.h>
#include <moya/libmoya/kernel/volume/Volume.h>

#include <cstdint>

namespace v3d::moya::kernel::volume {

/** Every volume the scene places, and the grids its fields read, each a NanoVDB buffer. **/
struct Volumes {
    Array<Volume> volumes;
    Array<GridPlace> grids;
    Array<Block> blocks;

    /** Whether the scene places any volume. **/
    MOYA_KERNEL bool any() const {
        return volumes.size != 0U;
    }

    /** The grid of a field of a volume. **/
    MOYA_KERNEL const nanovdb::FloatGrid & gridOf(const Volume & volume, std::uint32_t field) const {
        return gridIn(blocks, grids[volume.fields[field].grid]);
    }
};

};  // namespace v3d::moya::kernel::volume
