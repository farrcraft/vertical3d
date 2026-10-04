/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <api/render/offline/Film.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/sl/Imager.h>

#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "HitShader.h"

namespace v3d::talyn {

namespace {

// the planes of an RGBA framebuffer
const unsigned int RED = 0;
const unsigned int ALPHA = 3;

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

    int viewport[4] = { 0, 0, static_cast<int>(width), static_cast<int>(height) };

    // one of these for the render rather than one per pixel: it holds the register files,
    // and sizing one per pixel is the one allocation a tracer would notice
    HitShader shader(&scene_);

    // a sample at a time into the film, which filters them into pixels once every ray is
    // cast, per ADR-0076
    // a lens moves each sample's eye across it and aims the ray at the point it would have
    // reached on the plane of focus, so that plane stays sharp and nothing else does. An
    // orthographic camera has no lens to move
    const float radius = camera.orthographic() ? 0.0f : sampling_.lensRadius();
    // the camera's own axes and eye, from the view rather than the profile's normals, which
    // a rotation does not update
    const glm::mat4x4 toWorld = glm::inverse(camera.view());
    const glm::vec3 across = glm::normalize(glm::vec3(toWorld[0]));
    const glm::vec3 upward = glm::normalize(glm::vec3(toWorld[1]));
    const glm::vec3 forward = glm::normalize(glm::vec3(toWorld[2]));
    const glm::vec3 eye = glm::vec3(toWorld[3]);

    const v3d::render::offline::Sampler sampler(sampling_);
    v3d::render::offline::Film film(width, height, sampling_);
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            for (const v3d::render::offline::Sampler::Sample & at : sampler.pixel(column, row)) {
                // the camera measures y downward from the top of the viewport and image row 0
                // is the top of the picture, so a raster position is a screen point as it stands
                v3d::type::geometry::Ray ray = camera.ray(at.raster, viewport);
                if (radius > 0.0f) {
                    const glm::vec3 & origin = ray.origin();
                    const float along = sampling_.focalDistance - glm::dot(origin - eye, forward);
                    const glm::vec3 focus = origin + ray.direction() * (along / glm::dot(ray.direction(), forward));
                    const glm::vec3 moved = origin + radius * (at.lens.x * across + at.lens.y * upward);
                    ray = v3d::type::geometry::Ray(moved, focus - moved);
                }

                v3d::render::offline::Film::Sample sample;
                sample.raster = at.raster;
                sample.colour = scene_.background();
                Hit hit;
                sample.hit = scene_.nearest(ray, 0.0f, &hit, at.time);
                if (sample.hit) {
                    // the nearest hit is the batch, and the surface shader's Ci is the sample
                    shader.time(at.time);
                    sample.colour = shader.shade(hit);
                    sample.opacity = glm::vec3(1.0f);
                    sample.depth = hit.distance;
                }
                film.add(sample);
            }
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

};  // namespace v3d::talyn
