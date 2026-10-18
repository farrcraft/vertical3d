/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/Placed.h>

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya {

/**
 * What a primitive is shaded by: its surface shader and the lights that were switched on
 * when it was submitted.
 *
 * A primitive keeps a copy with its placement and its colour. A split resubmits its pieces
 * during the second pass, when none of the graphics state is current, and each piece is
 * shaded with its parent's copy.
 **/
class Shading final {
 public:
    v3d::render::offline::sl::InstancePtr surface;
    glm::mat4x4 placement = glm::mat4x4(1.0F);
    /**
     * The opacity that was current, SL's Os. The colour travels on the vertices because a
     * primitive may carry its own varying "Cs". moya reads no varying "Os", so this single
     * value covers the whole primitive.
     **/
    glm::vec3 opacity = glm::vec3(1.0F);
    /**
     * The lights that were switched on, each with the space the scene instanced it in. A
     * light shader states `from` and `to` in that space and shades points in eye space.
     * The pair places a light where the scene put it rather than at the origin.
     **/
    std::vector<v3d::render::offline::sl::Placed> lights;
};

};  // namespace v3d::moya
