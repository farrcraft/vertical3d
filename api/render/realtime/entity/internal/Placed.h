/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/MeshRegistry.h>

#include <cstdint>

#include <glm/mat4x4.hpp>

namespace v3d::render::realtime::entity::internal {

/**
 * An entity about to be drawn: its entry, where it stands, and where its palette starts.
 **/
struct Placed final {
    const MeshRegistry::Entry* entry;
    glm::mat4 model;
    uint32_t firstJoint;
};

};  // namespace v3d::render::realtime::entity::internal
