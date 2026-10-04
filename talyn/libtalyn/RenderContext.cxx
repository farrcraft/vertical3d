/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <api/render/offline/Film.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/sl/Imager.h>

#include <algorithm>
#include <array>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "HitShader.h"

namespace v3d::talyn {

namespace {

// the planes of an RGBA framebuffer
const unsigned int RED = 0;
const unsigned int ALPHA = 3;

/**
 * Casts one sample's ray and shades what it hits.
 *
 * A lens moves each sample's eye across it and aims the ray at the point it would have reached
 * on the plane of focus, so that plane stays sharp and nothing else does. An orthographic
 * camera has no lens to move.
 **/
class Caster {
 public:
    Caster(const Scene * scene, v3d::type::camera::Camera * camera, HitShader * shader,
        const v3d::render::offline::Sampling & sampling, const std::array<int, 4> & viewport) :
        scene_(scene), camera_(camera), shader_(shader), viewport_(viewport), focus_(sampling.focalDistance) {
        radius_ = camera->orthographic() ? 0.0f : sampling.lensRadius();
        // the camera's own axes and eye, from the view rather than the profile's normals,
        // which a rotation does not update
        const glm::mat4x4 toWorld = glm::inverse(camera->view());
        across_ = glm::normalize(glm::vec3(toWorld[0]));
        upward_ = glm::normalize(glm::vec3(toWorld[1]));
        forward_ = glm::normalize(glm::vec3(toWorld[2]));
        eye_ = glm::vec3(toWorld[3]);
    }

    v3d::render::offline::Film::Sample cast(const v3d::render::offline::Sampler::Sample & at) const {
        // the camera measures y downward from the top of the viewport and image row 0 is the
        // top of the picture, so a raster position is a screen point as it stands
        int viewport[4] = { viewport_[0], viewport_[1], viewport_[2], viewport_[3] };
        v3d::type::geometry::Ray ray = camera_->ray(at.raster, viewport);
        if (radius_ > 0.0f) {
            const glm::vec3 & origin = ray.origin();
            const float along = focus_ - glm::dot(origin - eye_, forward_);
            const glm::vec3 focus = origin + ray.direction() * (along / glm::dot(ray.direction(), forward_));
            const glm::vec3 moved = origin + radius_ * (at.lens.x * across_ + at.lens.y * upward_);
            ray = v3d::type::geometry::Ray(moved, focus - moved);
        }

        // what the ray sees through every surface it passes, over the background
        shader_->time(at.time);
        const HitShader::Seen seen = shader_->see(ray);
        v3d::render::offline::Film::Sample sample;
        sample.raster = at.raster;
        sample.colour = seen.colour;
        sample.opacity = seen.opacity;
        sample.hit = seen.hit;
        sample.depth = seen.distance;
        return sample;
    }

 private:
    const Scene * scene_;
    v3d::type::camera::Camera * camera_;
    HitShader * shader_;
    std::array<int, 4> viewport_;
    float focus_;
    float radius_ = 0.0f;
    glm::vec3 across_;
    glm::vec3 upward_;
    glm::vec3 forward_;
    glm::vec3 eye_;
};

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

RenderContext::RenderContext() {
}

void RenderContext::format(unsigned int width, unsigned int height) {
    // allocate a new 4 channel framebuffer object
    framebuffer_.reset(new v3d::render::offline::FrameBuffer(width, height, 4));
}

Scene & RenderContext::scene() {
    return scene_;
}

const Scene & RenderContext::scene() const {
    return scene_;
}

void RenderContext::render() {
    /*
    rendering algorithm:

    for each pixel in the framebuffer
        generate ray R with origin O at viewing position & passing through point on viewing plane for the current pixel
        pixel color = trace (R, 0)
        render pixel color into framebuffer

    trace (ray, depth)
        distance = maximum distance
        hit object = null
        for each object in scene
            calculate distance from intersection of R & current object
            if intersection distance < distance
                hit object = current object
                distance = intersection distance
        if hit object is null
            return background color
        color = black
        for each light source in scene
            for each object in scene
                if object blocks light between light source origin & intersection point
                    attenuate light intensity by transmittivity of object
            calculate color of hit object at intersection point with attenuated light intensity
            add calculated color to final color
        if depth < maximum depth
            generate reflection ray
            generate refraction ray
            trace (reflection, depth + 1)
            add color * object reflectivity to final color
            trace (refraction, depth + 1)
            add color * object transmittivity to final color
        return color
    */
    if (!framebuffer_) {
        return;
    }

    const unsigned int width = framebuffer_->width();
    const unsigned int height = framebuffer_->height();

    v3d::type::camera::Camera & camera = scene_.camera();
    camera.profile().size(width, height);
    camera.createProjection();
    camera.createView();

    const std::array<int, 4> viewport = { 0, 0, static_cast<int>(width), static_cast<int>(height) };

    // one of these for the render rather than one per pixel: it holds the register files,
    // and sizing one per pixel is the one allocation a tracer would notice
    HitShader shader(&scene_);

    // a sample at a time into the film, which filters them into pixels once every ray is
    // cast, per ADR-0076
    const Caster caster(&scene_, &camera, &shader, sampling_, viewport);
    const v3d::render::offline::Sampler sampler(sampling_);
    v3d::render::offline::Film film(width, height, sampling_);
    taken_.assign(static_cast<std::size_t>(width) * height, 0);
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            /*
                A PixelVariance above zero asks for another set wherever the first leaves the
                pixel uncertain: while the variance of the pixel's mean is above it, up to four
                times the first set. Each set is seeded by the pixel and its pass, so the
                answer does not depend on the order anything is rendered in.
            */
            Spread spread;
            unsigned int taken = 0;
            for (unsigned int pass = 0;; pass++) {
                const std::vector<v3d::render::offline::Sampler::Sample> set = sampler.pixel(column, row, pass);
                for (const v3d::render::offline::Sampler::Sample & at : set) {
                    const v3d::render::offline::Film::Sample sample = caster.cast(at);
                    film.add(sample);
                    spread.add(sample.colour);
                }
                taken += static_cast<unsigned int>(set.size());
                if (!(sampling_.variance > 0.0f) || taken >= 4 * set.size() || spread.settled(sampling_.variance)) {
                    break;
                }
            }
            taken_[static_cast<std::size_t>(row) * width + column] = taken;
        }
    }
    // a ray that hit nothing covered nothing, which is what lets an imager tell a pixel the
    // scene never reached from a black one
    film.resolve(framebuffer_.get(), RED, ALPHA);

    if (imager_.shader) {
        // after the last ray, which is where every sample the frame will ever hold is in
        // it - and it is the same place moya runs one, after its last bucket
        v3d::render::offline::sl::Imager imager(imager_.shader, &shader);
        imager.run(framebuffer_.get(), ALPHA);
    }
}

void RenderContext::imager(const v3d::render::offline::sl::Placed & shader) {
    imager_ = shader;
}

boost::shared_ptr<v3d::render::offline::FrameBuffer> RenderContext::framebuffer() const {
    return framebuffer_;
}

v3d::render::offline::Sampling & RenderContext::sampling() {
    return sampling_;
}

const v3d::render::offline::Sampling & RenderContext::sampling() const {
    return sampling_;
}

unsigned int RenderContext::samplesTaken(unsigned int column, unsigned int row) const {
    if (!framebuffer_ || column >= framebuffer_->width() || row >= framebuffer_->height() || taken_.empty()) {
        return 0;
    }
    return taken_[static_cast<std::size_t>(row) * framebuffer_->width() + column];
}

};  // namespace v3d::talyn
