/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pipeline.h"

namespace v3d::render::realtime::vulkan::pipeline {

/**
 **/
Pipeline::Pipeline() noexcept :
    pipeline(VK_NULL_HANDLE),
    layout(VK_NULL_HANDLE),
    pushStages(0),
    scene(false),
    biased(false),
    depthFormat(VK_FORMAT_UNDEFINED) {
}

};  // namespace v3d::render::realtime::vulkan::pipeline
