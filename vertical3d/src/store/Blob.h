/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace v3d::editor::store {

/**
 * One blob as a row of the buffers table holds it: how many entries the row says it holds, and
 * its bytes.
 **/
struct Blob {
    std::int64_t count = 0;
    std::vector<std::byte> data;
};

};  // namespace v3d::editor::store
