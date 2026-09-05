/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <limits>
#include <vector>

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

    v3d::type::Camera & camera = scene_.camera();
    camera.profile().size(width, height);
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, static_cast<int>(width), static_cast<int>(height) };

    const std::vector<Triangle> & triangles = scene_.triangles();

    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            // the camera measures y downward from the top of the viewport and image row 0
            // is the top of the picture, so a pixel index is a screen point as it stands
            glm::vec2 point(static_cast<float>(column) + 0.5f, static_cast<float>(row) + 0.5f);
            v3d::type::Ray ray = camera.ray(point, viewport);

            glm::vec3 colour = scene_.background();
            float nearest = std::numeric_limits<float>::max();
            for (const Triangle & triangle : triangles) {
                float distance = 0.0f;
                if (ray.intersects(triangle.a(), triangle.b(), triangle.c(), &distance) && distance < nearest) {
                    nearest = distance;
                    colour = triangle.colour();
                }
            }

            framebuffer_->value(RED, column, row, colour.r);
            framebuffer_->value(GREEN, column, row, colour.g);
            framebuffer_->value(BLUE, column, row, colour.b);
            // the whole frame is covered, background included, so nothing here is
            // transparent - an unwritten alpha plane is what made the old black png
            framebuffer_->value(ALPHA, column, row, 1.0f);
        }
    }
}

boost::shared_ptr<v3d::render::offline::FrameBuffer> RenderContext::framebuffer() const {
    return framebuffer_;
}

};  // namespace v3d::talyn
