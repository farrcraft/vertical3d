/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace v3d::render::realtime {

/**
 * How the window's images are presented. A settings screen calls this choice vsync.
 *
 * It is a request. A driver need not offer every mode, so each one falls back to a mode it does
 * offer, and the mode in use is reported back as one of these too. FIFO is the one mode every
 * driver must offer, so Vsync is always granted, and every request ends at it.
 **/
enum class Present : uint8_t {
    Uncapped,       /**< as fast as the device draws, without tearing (mailbox), else Vsync **/
    Vsync,          /**< one image a refresh, waiting for the display (fifo) **/
    AdaptiveVsync,  /**< as Vsync, but a late image is shown at once and may tear (fifo relaxed), else Vsync **/
    Immediate       /**< as soon as it is drawn, tearing (immediate), else Uncapped, else Vsync **/
};

/**
 * @return the vulkan present modes a request takes, best first. Fifo ends every list.
 **/
std::vector<VkPresentModeKHR> presentModes(Present present);

/**
 * @return the request a vulkan present mode answers. A mode no request names answers Vsync.
 **/
Present granted(VkPresentModeKHR mode) noexcept;

};  // namespace v3d::render::realtime
