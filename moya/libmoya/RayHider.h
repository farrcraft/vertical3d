/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Camera.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/trace/Scene.h>
#include <api/type/geometry/Ray.h>

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Hider.h"

namespace v3d::moya {

/**
 * The hider `Hider "raytrace"` selects: a primary ray through every sample, into the traced
 * scene, rather than primitives diced and bucketed.
 *
 * A ray is solved from the same camera to raster transformation the reyes hider projects
 * through, so the two hiders agree about where a pixel looks. The ray is then carried into
 * world space, where the traced scene is.
 **/
class RayHider final : public Hider {
 public:
    bool traces() const override;

    /**
     * Cast every sample of every pixel and write what the film makes of them.
     *
     * With a PixelVariance above zero, a pixel whose samples' variance of the mean is above it
     * takes further sets of samples, up to four sets in all.
     **/
    void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) override;

    /**
     * How many samples a pixel took in the last render: PixelSamples' count, or more where a
     * PixelVariance asked for them.
     **/
    unsigned int samplesTaken(const glm::uvec2 & pixel) const override;

    /**
     * Look through a camera, for the rays render() casts. A ray is solved from its raster
     * matrix, and starts at its hither. render() reads the camera from the context.
     **/
    void camera(const v3d::render::offline::Camera & camera);

    /**
     * The primary ray through a raster position, in world space, from a point on the unit
     * lens disc. An orthographic camera has no lens to move.
     **/
    v3d::type::geometry::Ray ray(const v3d::render::offline::Sampler::Sample & at,
        const v3d::render::offline::Sampling & sampling) const;

 private:
    /**
     * The camera space point at depth z that projects to a raster position.
     **/
    glm::vec3 unproject(const glm::vec2 & raster, float z) const;

    v3d::render::offline::Camera camera_;
    glm::mat4x4 toWorld_ = glm::mat4x4(1.0F);
    std::vector<unsigned int> taken_;
};

};  // namespace v3d::moya
