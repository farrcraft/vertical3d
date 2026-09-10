/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Instance.h>

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya {

/**
 * One light shining on a primitive: the shader instance a `LightSource` request made, and
 * where the scene put it.
 *
 * The placement is the light's own space into eye space - the transform that was in force
 * when the scene instanced it. A light shader states `from` and `to` in that space and
 * shades points in this one, so the pair is what makes a light placed by a transform land
 * where the scene put it rather than at the origin.
 **/
class Light final {
 public:
    v3d::render::offline::sl::InstancePtr shader;
    glm::mat4x4 placement = glm::mat4x4(1.0f);
};

/**
 * What a primitive is shaded by: its surface shader and the lights that were switched on
 * when it was submitted.
 *
 * A primitive keeps a copy for the reason it keeps its placement and its colour - a split
 * resubmits its pieces during the second pass, when none of the graphics state is current
 * any more, and a scene that shades two objects differently would otherwise shade a piece
 * of the first with the shader of the last.
 **/
class Shading final {
 public:
    v3d::render::offline::sl::InstancePtr surface;
    glm::mat4x4 placement = glm::mat4x4(1.0f);
    /**
     * The opacity that was current, which is SL's Os. The colour travels on the vertices
     * because a primitive may carry its own varying "Cs"; there is no varying opacity, so
     * this one value is the whole of it.
     **/
    glm::vec3 opacity = glm::vec3(1.0f);
    std::vector<Light> lights;
};

};  // namespace v3d::moya
