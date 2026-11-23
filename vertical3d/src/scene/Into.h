/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Timeline.h"

namespace v3d::editor::scene {

/**
 * What an import is made to fit: the timeline its keys go on, and the unit its lengths are in.
 **/
struct Into final {
    float rate = Timeline::DEFAULT_RATE;  /**< the frames a second of the timeline the keys go on **/
    double metres = 1.0;                  /**< how many metres one unit of the scene is **/
};

};  // namespace v3d::editor::scene
