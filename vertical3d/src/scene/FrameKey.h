/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::editor::scene {

/**
 * One item's keys on one frame, which the timeline shows, moves and deletes as one key.
 **/
struct FrameKey final {
    unsigned int item = 0;  /**< the item's id in the scene **/
    float frame = 0.0F;

    bool operator==(const FrameKey& other) const = default;
};

};  // namespace v3d::editor::scene
