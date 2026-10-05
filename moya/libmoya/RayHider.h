/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
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
 * A ray is found by inverting the same camera to raster transformation the reyes hider
 * projects through, so the two hiders cannot disagree about where a pixel looks; it is then
 * carried into world space, where the traced scene is.
 **/
class RayHider final : public Hider {
 public:
    /**
     * The camera as RiWorldBegin froze it.
     **/
    class Camera final {
     public:
        /** The world to camera transformation. **/
        glm::mat4x4 toCamera = glm::mat4x4(1.0f);
        /** Camera to raster, which a ray is found by inverting and a depth is projected through. **/
        glm::mat4x4 toRaster = glm::mat4x4(1.0f);
        bool perspective = false;
        /** Where a primary ray starts, in camera space z. **/
        float hither = 0.0f;
        unsigned int width = 0;
        unsigned int height = 0;
    };

    bool traces() const override;

    /**
     * Cast every sample of every pixel and write what the film makes of them.
     *
     * With a PixelVariance above zero, another set of samples is taken wherever the first
     * leaves a pixel uncertain.
     **/
    void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) override;

    /**
     * How many samples a pixel took in the last render: PixelSamples' count, or more where a
     * PixelVariance asked for them.
     **/
    unsigned int samplesTaken(unsigned int column, unsigned int row) const override;

    /**
     * Look through a camera, for the rays render() casts.
     **/
    void camera(const Camera & camera);

    /**
     * The primary ray through a raster position, in world space, from a point on the unit
     * lens disc. An orthographic camera has no lens to move.
     **/
    v3d::type::geometry::Ray ray(const glm::vec2 & raster, const glm::vec2 & lens,
        const v3d::render::offline::Sampling & sampling) const;

 private:
    /**
     * The camera space point at depth z that projects to a raster position.
     **/
    glm::vec3 unproject(const glm::vec2 & raster, float z) const;

    Camera camera_;
    glm::mat4x4 toWorld_ = glm::mat4x4(1.0f);
    std::vector<unsigned int> taken_;
};

};  // namespace v3d::moya
