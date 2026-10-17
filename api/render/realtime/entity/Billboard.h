/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::realtime::entity {

/**
 * How a quad facing the camera is laid out, and how it is sorted.
 *
 * A quad's width runs along right and its height along up, which are the camera's. Its key in
 * a depth order is how far it lies along depthAxis, larger being further. The caller chooses the
 * axis: the camera's forward flattened onto the ground for an orthographic view of a ground
 * plane, or the view direction for a perspective one.
 **/
struct Billboard {
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 depthAxis{0.0f, 0.0f, 1.0f};
};

};  // namespace v3d::render::realtime::entity
