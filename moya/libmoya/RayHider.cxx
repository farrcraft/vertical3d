/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RayHider.h"

#include <api/render/offline/Camera.h>
#include <api/render/offline/Film.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/type/geometry/Ray.h>

#include <algorithm>
#include <cstddef>
#include <functional>
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

/**
 * One pixel of a row as the ray hider casts it: the samples it has taken so far, in the order
 * it took them, and whether it needs no more.
 **/
class Pixel final {
 public:
    Spread spread;
    unsigned int taken = 0;
    bool settled = false;
    std::vector<v3d::render::offline::Film::Sample> samples;
};

/** Which pass of which row is being cast, and the first column of the row's pixels. **/
class Pass final {
 public:
    unsigned int row = 0;
    unsigned int pass = 0;
    unsigned int left = 0;
};

/**
 * Cast one pass of a row: a set of samples for every pixel of it still unsettled, as one batch,
 * so their hits are shaded together. Each sample is kept on its pixel, and each pixel then
 * settles or does not.
 *
 * @param depth the raster depth of a world space point, for the depth plane
 * @return false when every pixel had settled, so that nothing was cast
 **/
bool cast(const RayHider & hider, v3d::render::offline::trace::Tracer* tracer, const v3d::render::offline::Sampler & sampler,
    const v3d::render::offline::Sampling & sampling, const Pass & at, const std::function<float(const glm::vec3 &)> & depth,
    std::vector<Pixel>* pixels) {
    std::vector<v3d::render::offline::trace::Tracer::Look> looks;
    std::vector<v3d::render::offline::Sampler::Sample> cast;
    std::vector<std::size_t> owners;
    std::vector<unsigned int> sets(pixels->size(), 0);
    for (std::size_t index = 0; index < pixels->size(); index++) {
        if ((*pixels)[index].settled) {
            continue;
        }
        const std::vector<v3d::render::offline::Sampler::Sample> set =
            sampler.pixel(at.left + static_cast<unsigned int>(index), at.row, at.pass);
        sets[index] = static_cast<unsigned int>(set.size());
        for (const v3d::render::offline::Sampler::Sample & sample : set) {
            v3d::render::offline::trace::Tracer::Look look;
            look.ray = hider.ray(sample, sampling);
            look.time = sample.time;
            looks.push_back(look);
            cast.push_back(sample);
            owners.push_back(index);
        }
    }
    if (looks.empty()) {
        return false;
    }

    const std::vector<v3d::render::offline::trace::Tracer::Seen> seen = tracer->see(looks);
    for (std::size_t i = 0; i < looks.size(); i++) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = cast[i].raster;
        sample.colour = seen[i].colour;
        sample.opacity = seen[i].opacity;
        sample.hit = seen[i].hit;
        if (seen[i].hit) {
            sample.depth = depth(looks[i].ray.origin() + looks[i].ray.direction() * seen[i].distance);
        }
        Pixel & pixel = (*pixels)[owners[i]];
        pixel.samples.push_back(sample);
        pixel.spread.add(sample.colour);
    }
    for (std::size_t index = 0; index < pixels->size(); index++) {
        Pixel & pixel = (*pixels)[index];
        if (pixel.settled) {
            continue;
        }
        pixel.taken += sets[index];
        pixel.settled = !(sampling.variance > 0.0F) || pixel.taken >= 4 * sets[index] || pixel.spread.settled(sampling.variance);
    }
    return true;
}

};  // namespace

bool RayHider::traces() const {
    return true;
}

void RayHider::camera(const v3d::render::offline::Camera & camera) {
    camera_ = camera;
    toWorld_ = glm::inverse(camera.view);
}

glm::vec3 RayHider::unproject(const glm::vec2 & raster, float z) const {
    // a raster position is (matrix * p).xy / (matrix * p).w. With p's z fixed, the numerator
    // and w are both linear in p's x and y, which gives two equations in two unknowns. Solving
    // them at a depth keeps the answer clear of the projection's depth terms. RI's default
    // clipping range makes those terms as badly conditioned as a float allows, so the whole
    // matrix is not inverted
    const glm::mat4x4 & matrix = camera_.raster;
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
    v3d::render::offline::Camera camera = context->camera();
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
    // no ray is cast outside the crop window. The context clears those pixels afterwards
    const glm::uvec4 window = context->cropped();
    const unsigned int left = window.x;
    const unsigned int right = std::min(window.y, width);
    // the depth plane holds raster space z, as the reyes hider's does
    const std::function<float(const glm::vec3 &)> rasterDepth = [this](const glm::vec3 & point) {
        const glm::vec3 eye(camera_.view * glm::vec4(point, 1.0F));
        return project(camera_.raster, eye).z;
    };
    /*
        A row is cast pass by pass, every unsettled pixel's set of samples as one batch. Each
        set of samples is seeded by the pixel and its pass, so what a pixel sees does not depend
        on the batch it was cast in. The film is given the samples pixel by pixel afterwards, in
        the order a pixel at a time would give them, so its sums are the same.
    */
    std::vector<Pixel> pixels;
    for (unsigned int row = window.z; row < window.w && row < height; row++) {
        pixels.assign(right > left ? right - left : 0U, Pixel());
        unsigned int pass = 0;
        while (cast(*this, &tracer, sampler, sampling, {.row = row, .pass = pass, .left = left}, rasterDepth, &pixels)) {
            pass++;
        }
        for (std::size_t index = 0; index < pixels.size(); index++) {
            for (const v3d::render::offline::Film::Sample & sample : pixels[index].samples) {
                film.add(sample);
            }
            taken_[(static_cast<std::size_t>(row) * width) + left + index] = pixels[index].taken;
        }
        // the rows already traced are resolved, so a stopped render keeps what it found
        if (!context->report(static_cast<float>(row + 1 - window.z) / static_cast<float>(window.w - window.z))) {
            break;
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
