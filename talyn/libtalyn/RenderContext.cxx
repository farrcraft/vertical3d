/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <api/render/offline/sl/Imager.h>

#include <vector>

#include "HitShader.h"

namespace v3d::talyn {

namespace {

// the planes of an RGBA framebuffer
const unsigned int RED = 0;
const unsigned int GREEN = 1;
const unsigned int BLUE = 2;
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

    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            // the camera measures y downward from the top of the viewport and image row 0
            // is the top of the picture, so a pixel index is a screen point as it stands
            glm::vec2 point(static_cast<float>(column) + 0.5f, static_cast<float>(row) + 0.5f);
            v3d::type::geometry::Ray ray = camera.ray(point, viewport);

            glm::vec3 colour = scene_.background();
            Hit hit;
            const bool covered = scene_.nearest(ray, 0.0f, &hit);
            if (covered) {
                // the nearest hit is the batch, and the surface shader's Ci is the pixel
                colour = shader.shade(hit);
            }

            framebuffer_->value(RED, column, row, colour.r);
            framebuffer_->value(GREEN, column, row, colour.g);
            framebuffer_->value(BLUE, column, row, colour.b);
            // a ray that hit nothing covered nothing, which is what lets an imager tell
            // a pixel the scene never reached from a black one
            framebuffer_->value(ALPHA, column, row, covered ? 1.0f : 0.0f);
        }
    }

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

};  // namespace v3d::talyn
