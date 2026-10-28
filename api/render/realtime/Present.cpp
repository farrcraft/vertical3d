/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Present.h"

#include <vector>

namespace v3d::render::realtime {

/**
 **/
std::vector<VkPresentModeKHR> presentModes(Present present) {
    switch (present) {
    case Present::Uncapped:
        return {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR};
    case Present::Vsync:
        return {VK_PRESENT_MODE_FIFO_KHR};
    case Present::AdaptiveVsync:
        return {VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR};
    case Present::Immediate:
        return {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR};
    }
    return {VK_PRESENT_MODE_FIFO_KHR};
}

/**
 **/
Present granted(VkPresentModeKHR mode) noexcept {
    switch (mode) {
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return Present::Uncapped;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return Present::AdaptiveVsync;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return Present::Immediate;
    default:
        return Present::Vsync;
    }
}

};  // namespace v3d::render::realtime
