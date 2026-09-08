/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include <glm/mat4x4.hpp>

namespace v3d::render::offline {

/**
 * What the machine needs from a renderer, and does not hold itself.
 *
 * A shader run knows nothing about buckets, grids, rays or scenes. What it does need - the
 * matrix for a named coordinate space, and later the active lights, whether light reaches a
 * point, and a ray traced - arrives through this, which moya and talyn each implement.
 *
 * Every method has an answer for a renderer that cannot do it, because the two renderers
 * genuinely disagree: talyn traces a shadow ray and moya answers that light gets through
 * until it has a shadow map.
 **/
class SLRenderer {
 public:
    virtual ~SLRenderer() = default;

    /**
     * The matrix from the shader's current space into the named one.
     *
     * moya's current space is camera space and talyn's is world space, which is why this is
     * a callback rather than a table the library holds.
     *
     * @param name the space: "current", "object", "shader", "world", "camera", "raster", ...
     * @param matrix where to put it, if the renderer knows the space
     * @return whether it did; a space a renderer does not know leaves the value alone
     **/
    virtual bool space(const std::string & name, glm::mat4x4* matrix) = 0;
};

};  // namespace v3d::render::offline
