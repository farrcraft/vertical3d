/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RayHider.h"

#include <api/render/offline/Film.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/type/geometry/Ray.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "FrameBuffer.h"
#include "RenderContext.h"

namespace v3d::moya {

namespace {

/**
 * How far a pixel's samples disagree: the variance of their mean, per channel, which RI's
 * PixelVariance bounds.
 **/
class Spread {
 public:
    void add(const glm::vec3 & colour) {
        count_++;
        sum_ += glm::dvec3(colour);
        squares_ += glm::dvec3(colour) * glm::dvec3(colour);
    }

    /**
     * Whether the largest channel's variance of the mean is within the bound. Fewer than two
     * samples give no measure of spread, so they are never settled.
     **/
    bool settled(float bound) const {
        if (count_ < 2) {
            return false;
        }
        const double samples = static_cast<double>(count_);
        const glm::dvec3 mean = sum_ / samples;
        const glm::dvec3 variance = (squares_ - mean * mean * samples) / (samples - 1.0);
        const double worst = std::max(variance.x, std::max(variance.y, variance.z));
        return worst / samples <= static_cast<double>(bound);
    }

 private:
    unsigned int count_ = 0;
    glm::dvec3 sum_ { 0.0 };
    glm::dvec3 squares_ { 0.0 };
};

};  // namespace

bool RayHider::traces() const {
    return true;
}

void RayHider::camera(const Camera & camera) {
    camera_ = camera;
    toWorld_ = glm::inverse(camera.toCamera);
}

glm::vec3 RayHider::unproject(const glm::vec2 & raster, float z) const {
    // a raster position is (matrix * p).xy / (matrix * p).w. With p's z fixed, the numerator
    // and w are both linear in p's x and y, which gives two equations in two unknowns. Solving
    // them at a depth keeps the answer clear of the projection's depth terms. RI's default
    // clipping range makes those terms as badly conditioned as a float allows, so the whole
    // matrix is not inverted
    const glm::mat4x4 & matrix = camera_.toRaster;
    // the system is A (x, y) = b, where a row of A and of b is one raster coordinate's equation
    const float a00 = matrix[0][0] - (raster.x * matrix[0][3]);
    const float a01 = matrix[1][0] - (raster.x * matrix[1][3]);
    const float a10 = matrix[0][1] - (raster.y * matrix[0][3]);
    const float a11 = matrix[1][1] - (raster.y * matrix[1][3]);
    const float b0 = (raster.x * ((matrix[2][3] * z) + matrix[3][3])) - ((matrix[2][0] * z) + matrix[3][0]);
    const float b1 = (raster.y * ((matrix[2][3] * z) + matrix[3][3])) - ((matrix[2][1] * z) + matrix[3][1]);
    const float determinant = (a00 * a11) - (a01 * a10);
    if (determinant == 0.0F) {
        return glm::vec3(0.0F, 0.0F, z);
    }
    return glm::vec3(((b0 * a11) - (a01 * b1)) / determinant, ((a00 * b1) - (b0 * a10)) / determinant, z);
}

v3d::type::geometry::Ray RayHider::ray(const v3d::render::offline::Sampler::Sample & at,
    const v3d::render::offline::Sampling & sampling) const {
    const glm::vec2 & raster = at.raster;
    const glm::vec2 & lens = at.lens;
    glm::vec3 origin;
    glm::vec3 direction;
    if (camera_.perspective) {
        // every point a perspective camera sees at a raster position lies on one line from
        // the eye, so the point at depth one is the direction. A lens moves the eye across
        // the lens and aims at the point that line reaches on the plane of focus, so that
        // plane stays sharp and nothing else does
        direction = unproject(raster, 1.0F);
        origin = glm::vec3(0.0F);
        const float radius = sampling.lensRadius();
        if (radius > 0.0F) {
            const glm::vec3 focus = direction * sampling.focalDistance;
            origin = glm::vec3(radius * lens, 0.0F);
            direction = focus - origin;
        }
        // started on the near plane, which clips anything nearer the eye
        origin += direction * (camera_.hither / direction.z);
    } else {
        origin = unproject(raster, camera_.hither);
        direction = glm::vec3(0.0F, 0.0F, 1.0F);
    }
    return v3d::type::geometry::Ray({.origin = glm::vec3(toWorld_ * glm::vec4(origin, 1.0F)), .direction = glm::vec3(toWorld_ * glm::vec4(direction, 0.0F))});
}

void RayHider::render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) {
    Camera camera;
    camera.toCamera = context->coordinateSystem("camera");
    camera.toRaster = context->coordinateSystem("raster") * context->coordinateSystem("screen");
    camera.perspective = context->perspective();
    camera.hither = context->hither();
    camera.width = planes->width();
    camera.height = planes->height();
    this->camera(camera);

    const v3d::render::offline::Sampling & sampling = context->sampling();
    const unsigned int colour = FrameBuffer::RED;
    const unsigned int coverage = FrameBuffer::COVERAGE;
    const unsigned int depth = FrameBuffer::DEPTH;
    const unsigned int width = camera_.width;
    const unsigned int height = camera_.height;
    taken_.assign(static_cast<std::size_t>(width) * height, 0);

    // one tracer for the whole render, not one per pixel. It holds the register files, so
    // they are sized once and not for every pixel
    v3d::render::offline::trace::Tracer tracer(&context->traced(), &context->textures());
    const v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(width, height, sampling);
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            // each set of samples is seeded by the pixel and its pass, so the result does not
            // depend on the order the pixels are rendered in
            Spread spread;
            unsigned int taken = 0;
            for (unsigned int pass = 0;; pass++) {
                const std::vector<v3d::render::offline::Sampler::Sample> set = sampler.pixel(column, row, pass);
                for (const v3d::render::offline::Sampler::Sample & at : set) {
                    const v3d::type::geometry::Ray traced = ray(at, sampling);
                    tracer.time(at.time);
                    const v3d::render::offline::trace::Tracer::Seen seen = tracer.see(traced);

                    v3d::render::offline::Film::Sample sample;
                    sample.raster = at.raster;
                    sample.colour = seen.colour;
                    sample.opacity = seen.opacity;
                    sample.hit = seen.hit;
                    if (seen.hit) {
                        // the depth plane holds raster space z, as the reyes hider's does
                        const glm::vec3 point = traced.origin() + traced.direction() * seen.distance;
                        const glm::vec3 eye(camera_.toCamera * glm::vec4(point, 1.0F));
                        sample.depth = project(camera_.toRaster, eye).z;
                    }
                    film.add(sample);
                    spread.add(sample.colour);
                }
                taken += static_cast<unsigned int>(set.size());
                if (!(sampling.variance > 0.0F) || taken >= 4 * set.size() || spread.settled(sampling.variance)) {
                    break;
                }
            }
            taken_[(static_cast<std::size_t>(row) * width) + column] = taken;
        }
    }
    film.resolve(planes, colour, coverage, static_cast<int>(depth));
}

unsigned int RayHider::samplesTaken(const glm::uvec2 & pixel) const {
    if (pixel.x >= camera_.width || pixel.y >= camera_.height || taken_.empty()) {
        return 0;
    }
    return taken_[(static_cast<std::size_t>(pixel.y) * camera_.width) + pixel.x];
}

};  // namespace v3d::moya
