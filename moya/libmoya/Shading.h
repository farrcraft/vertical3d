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
 * A primitive keeps a copy with its placement and its colour. A split primitive resubmits
 * its pieces while the buckets render, after WorldEnd, when none of the graphics state is
 * current. Each piece is shaded with its parent's copy.
 **/
class Shading final {
 public:
    v3d::render::offline::sl::InstancePtr surface;
    glm::mat4x4 placement = glm::mat4x4(1.0F);
    /**
     * The primitive's object space into camera space, as the transformation stood when it was
     * submitted. SL's "object" space. It differs from the surface's placement when the scene
     * transforms between the surface and the primitive.
     **/
    glm::mat4x4 object = glm::mat4x4(1.0F);
    /**
     * The opacity that was current, SL's Os. The colour travels on the vertices because a
     * primitive may carry its own varying "Cs". moya reads no varying "Os", so this single
     * value covers the whole primitive.
     **/
    glm::vec3 opacity = glm::vec3(1.0F);
    /**
     * The lights that were switched on, each with the space the scene instanced it in. A
     * light shader states `from` and `to` in that space, and shades points in eye space.
     * The stored space places each light where the scene put it, not at the origin.
     **/
    std::vector<v3d::render::offline::sl::Placed> lights;
    /**
     * The displacement shader that was current, and the space it was instanced in. Null when
     * the scene named none.
     **/
    v3d::render::offline::sl::Placed displacement;
    /**
     * How far the displacement shader may move a point, `Attribute "displacementbound"
     * "sphere"`, in camera space. Zero when the scene gave no bound.
     **/
    float displacementBound = 0.0F;
};

};  // namespace v3d::moya
