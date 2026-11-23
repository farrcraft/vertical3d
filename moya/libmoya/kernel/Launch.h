/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel {

/**
 * One work item of a launch: a pixel of the image launched over, in pixels from its upper left.
 **/
struct Item {
    std::uint32_t x { 0 };
    std::uint32_t y { 0 };
};

/**
 * A kernel function a device launches, once for each item. Its parameters are read and never
 * written, so items run in any order and on any thread. An item writes only what is its own,
 * such as its own pixel, so the order items run in cannot change a result.
 **/
template <typename Params>
using Function = void (*)(const Params & params, Item item);

};  // namespace v3d::moya::kernel
