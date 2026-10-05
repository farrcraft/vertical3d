/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * An image as SL's `texture()` reads it: colours in [0, 1], sampled bilinearly between
 * texel centres and wrapped periodically, which are RI's defaults.
 *
 * s runs left to right and t top to bottom, so (0, 0) is the image's upper left corner.
 **/
class Texture final {
 public:
    /** A grey image is read into all three channels, and an alpha channel is dropped. **/
    explicit Texture(const v3d::image::Image & image);

    glm::vec3 sample(float s, float t) const;

    unsigned int width() const;
    unsigned int height() const;

 private:
    /** The texel at a column and row, either of which may be outside the image. **/
    const glm::vec3 & texel(int column, int row) const;

    unsigned int width_ = 0;
    unsigned int height_ = 0;
    std::vector<glm::vec3> texels_;
};

};  // namespace v3d::render::offline
