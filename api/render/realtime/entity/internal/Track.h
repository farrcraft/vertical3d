/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::render::realtime::entity::internal {

/** A clip, how far into it the playback is, how long it lasts, and whether it loops. **/
struct Track {
    uint32_t clip = 0;
    float time = 0.0F;
    float duration = 0.0F;
    bool loops = false;
};

};  // namespace v3d::render::realtime::entity::internal
