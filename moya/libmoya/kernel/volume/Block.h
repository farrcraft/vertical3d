/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/volume/Nano.h>

#include <cstdint>

namespace v3d::moya::kernel::volume {

/** A run of bytes as long as NanoVDB's alignment, of which a grid's buffer is held. **/
struct alignas(NANOVDB_DATA_ALIGNMENT) Block {
    std::uint64_t words[NANOVDB_DATA_ALIGNMENT / sizeof(std::uint64_t)];
};

};  // namespace v3d::moya::kernel::volume
