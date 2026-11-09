/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include "Image.h"
#include "Layer.h"

#include <glm/vec2.hpp>

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

/**
 * Colour glyphs: the layers, fills and images a face's colour tables give a glyph, and painting
 * them into bitmaps.
 **/
namespace v3d::font::glyph::colour {

/**
 * A colour glyph, as layers of filled outlines and images in em units, with y upwards.
 **/
struct Glyph final {
    std::vector<Layer> layers;
    std::vector<Image> images;
    glm::vec2 min{ 0.0F };  /**< the lower left of what it draws, in em units **/
    glm::vec2 max{ 0.0F };

    /**
     * @return whether it draws nothing
     **/
    bool empty() const noexcept;

    /**
     * @return whether any layer is in the text's colour
     **/
    bool foreground() const noexcept;
};

/**
 * Read a colour glyph from a face's colour tables, in a palette, at a variable font's normalized
 * coordinates. An image is taken from the strike nearest a size in pixels to the em. A palette
 * the face does not have is its first.
 *
 * @return the glyph, or an empty one when the face has no colour glyph at that index
 **/
Glyph read(const face::Face& face, unsigned int index, unsigned int palette, float size, const std::vector<std::int32_t>& coordinates = {});

};  // namespace v3d::font::glyph::colour
