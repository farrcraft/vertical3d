/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::render::realtime {

/**
 * How the window's images are presented, which is what a settings screen calls vsync.
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

};  // namespace v3d::render::realtime
