/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::editor::store {

/**
 * One blob a kind writes: its role, how many entries it holds, and its bytes.
 **/
struct Buffer {
    std::string role;
    std::size_t count = 0;
    std::vector<std::byte> data;
};

};  // namespace v3d::editor::store
