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

namespace v3d::moya {

/**
 * The hider `Hider "raytrace"` selects, per ADR-0078: a primary ray through every sample,
 * into the traced scene, rather than primitives diced and bucketed.
 *
 * A ray is built in camera space from the screen window and the projection, the same ones the
 * reyes hider projects through, and carried into world space, where the traced scene is.
 **/
class RayHider final {
 public:
    /**
     * The camera as RiWorldBegin froze it.
     **/
    class Camera final {
     public:
        /** The world to camera transformation. **/
        glm::mat4x4 toCamera = glm::mat4x4(1.0f);
        /** Camera to raster, which is what a depth is projected through. **/
        glm::mat4x4 toRaster = glm::mat4x4(1.0f);
        bool perspective = false;
        /** RI's fov, in degrees, spanning screen space [-1, 1]. **/
        float fov = 90.0f;
        /** Left, right, bottom and top, in screen space. **/
        float screen[4] = { -1.0f, 1.0f, -1.0f, 1.0f };
        /** Where a primary ray starts, in camera space z. **/
        float near = 0.0f;
        unsigned int width = 0;
        unsigned int height = 0;
    };

    explicit RayHider(const Camera & camera);

    /**
     * Cast every sample of every pixel and write what the film makes of them.
     *
     * A PixelVariance above zero asks for another set wherever the first leaves a pixel
     * uncertain, per ADR-0076.
     *
     * @param colour the first of the three planes the colour goes into
     **/
    void render(const v3d::render::offline::trace::Scene & scene, v3d::render::offline::Textures * textures,
        const v3d::render::offline::Sampling & sampling, v3d::render::offline::FrameBuffer * planes,
        unsigned int colour, unsigned int coverage, unsigned int depth);

    /**
     * The primary ray through a raster position, in world space, from a point on the unit
     * lens disc. An orthographic camera has no lens to move.
     **/
    v3d::type::geometry::Ray ray(const glm::vec2 & raster, const glm::vec2 & lens,
        const v3d::render::offline::Sampling & sampling) const;

    /**
     * How many samples a pixel took in the last render: PixelSamples' count, or more where a
     * PixelVariance asked for them.
     **/
    unsigned int samplesTaken(unsigned int column, unsigned int row) const;

 private:
    Camera camera_;
    glm::mat4x4 toWorld_;
    std::vector<unsigned int> taken_;
};

};  // namespace v3d::moya
