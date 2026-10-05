/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Allocation.h"

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
Allocation::Allocation() noexcept :
    memory(VK_NULL_HANDLE),
    offset(0),
    size(0),
    handle(VK_NULL_HANDLE) {
}

/**
 **/
bool Allocation::valid() const noexcept {
    return memory != VK_NULL_HANDLE;
}

};  // namespace v3d::render::realtime::vulkan::memory
