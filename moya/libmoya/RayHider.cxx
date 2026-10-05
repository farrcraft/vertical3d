/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RayHider.h"

#include <api/render/offline/Film.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/trace/Tracer.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

#include "RenderContext.h"

namespace v3d::moya {

namespace {

/**
 * How far a pixel's samples disagree: the variance of their mean, per channel, which is what
 * RI's PixelVariance bounds.
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
     * samples say nothing about their spread, so they are never settled.
     **/
    bool settled(float bound) const {
        if (count_ < 2) {
            return false;
        }
        const double n = static_cast<double>(count_);
        const glm::dvec3 mean = sum_ / n;
        const glm::dvec3 variance = (squares_ - mean * mean * n) / (n - 1.0);
        const double worst = std::max(variance.x, std::max(variance.y, variance.z));
        return worst / n <= static_cast<double>(bound);
    }

 private:
    unsigned int count_ = 0;
    glm::dvec3 sum_ { 0.0 };
    glm::dvec3 squares_ { 0.0 };
};

};  // namespace

RayHider::RayHider(const Camera & camera) : camera_(camera), toWorld_(glm::inverse(camera.toCamera)) {
}

v3d::type::geometry::Ray RayHider::ray(const glm::vec2 & raster, const glm::vec2 & lens,
    const v3d::render::offline::Sampling & sampling) const {
    // raster space counts y downward from the upper left, so its top row is the screen
    // window's top edge
    const float left = camera_.screen[0];
    const float right = camera_.screen[1];
    const float bottom = camera_.screen[2];
    const float top = camera_.screen[3];
    const float x = left + (right - left) * raster.x / static_cast<float>(camera_.width);
    const float y = top - (top - bottom) * raster.y / static_cast<float>(camera_.height);

    glm::vec3 origin;
    glm::vec3 direction;
    if (camera_.perspective) {
        /*
            A camera space point at depth z reaches screen x at x = sx * z * tan(fov / 2), so
            the ray through a screen point leaves the eye along (sx * t, sy * t, 1). A lens
            moves the eye across it and aims at the point that ray reaches on the plane of
            focus, so that plane stays sharp and nothing else does.
        */
        const float tangent = std::tan(glm::radians(camera_.fov) / 2.0f);
        direction = glm::vec3(x * tangent, y * tangent, 1.0f);
        origin = glm::vec3(0.0f);
        const float radius = sampling.lensRadius();
        if (radius > 0.0f) {
            const glm::vec3 focus = direction * sampling.focalDistance;
            origin = glm::vec3(radius * lens, 0.0f);
            direction = focus - origin;
        }
        // started on the near plane, which is what clips what is nearer the eye
        origin += direction * (camera_.near / direction.z);
    } else {
        origin = glm::vec3(x, y, camera_.near);
        direction = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return v3d::type::geometry::Ray(glm::vec3(toWorld_ * glm::vec4(origin, 1.0f)),
        glm::vec3(toWorld_ * glm::vec4(direction, 0.0f)));
}

void RayHider::render(const v3d::render::offline::trace::Scene & scene, v3d::render::offline::Textures * textures,
    const v3d::render::offline::Sampling & sampling, v3d::render::offline::FrameBuffer * planes,
    unsigned int colour, unsigned int coverage, unsigned int depth) {
    const unsigned int width = camera_.width;
    const unsigned int height = camera_.height;
    taken_.assign(static_cast<std::size_t>(width) * height, 0);

    // one of these for the render rather than one per pixel: it holds the register files,
    // and sizing one per pixel is the one allocation a tracer would notice
    v3d::render::offline::trace::Tracer tracer(&scene, textures);
    const v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(width, height, sampling);
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            // each further set is seeded by the pixel and its pass, so the answer does not
            // depend on the order anything is rendered in
            Spread spread;
            unsigned int taken = 0;
            for (unsigned int pass = 0;; pass++) {
                const std::vector<v3d::render::offline::Sampler::Sample> set = sampler.pixel(column, row, pass);
                for (const v3d::render::offline::Sampler::Sample & at : set) {
                    const v3d::type::geometry::Ray traced = ray(at.raster, at.lens, sampling);
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
                        const glm::vec3 eye(camera_.toCamera * glm::vec4(point, 1.0f));
                        sample.depth = project(camera_.toRaster, eye).z;
                    }
                    film.add(sample);
                    spread.add(sample.colour);
                }
                taken += static_cast<unsigned int>(set.size());
                if (!(sampling.variance > 0.0f) || taken >= 4 * set.size() || spread.settled(sampling.variance)) {
                    break;
                }
            }
            taken_[static_cast<std::size_t>(row) * width + column] = taken;
        }
    }
    film.resolve(planes, colour, coverage, static_cast<int>(depth));
}

unsigned int RayHider::samplesTaken(unsigned int column, unsigned int row) const {
    if (column >= camera_.width || row >= camera_.height || taken_.empty()) {
        return 0;
    }
    return taken_[static_cast<std::size_t>(row) * camera_.width + column];
}

};  // namespace v3d::moya
