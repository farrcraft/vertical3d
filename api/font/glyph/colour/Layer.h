/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Clip.h"
#include "Fill.h"

namespace v3d::font::glyph::colour {

/**
 * How a group is put on what is beneath it, as COLR version 1 names the modes. The first are
 * Porter and Duff's, and the rest blend colours as CSS's blend modes do.
 **/
enum class Composite : std::uint8_t {
    Clear, Source, Destination, SourceOver, DestinationOver, SourceIn, DestinationIn, SourceOut,
    DestinationOut, SourceAtop, DestinationAtop, Xor, Plus,
    Screen, Overlay, Darken, Lighten, ColourDodge, ColourBurn, HardLight, SoftLight, Difference,
    Exclusion, Multiply, Hue, Saturation, Colour, Luminosity
};

/**
 * One step of drawing a colour glyph.
 *
 * A fill or an image is drawn inside every one of its clips. A group gathers the steps between
 * its beginning and its end apart, then is put on what is beneath it by its composite. Steps
 * outside any group, and within one, are drawn over each other in order.
 **/
struct Layer final {
    enum class Kind : std::uint8_t {
        Fill,
        Image,
        BeginGroup,
        EndGroup
    };

    Kind kind = Kind::Fill;
    std::vector<Clip> clips{};
    Fill fill{};
    std::size_t image = 0;                           /**< an image's index in its glyph **/
    Composite composite = Composite::SourceOver;  /**< how an ended group is put down **/
    float opacity = 1.0F;                                     /**< what an ended group's colours are multiplied by as it is put down **/
};

};  // namespace v3d::font::glyph::colour
