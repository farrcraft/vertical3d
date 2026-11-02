/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Curves.h"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

namespace v3d::font::glyph {


/**
 * The colour tables a face holds. A face may hold several, and a glyph is drawn from the first
 * of COLR version 1, COLR version 0, SVG, then a bitmap table that has it.
 **/
struct ColourTables final {
    bool layers = false;  /**< COLR version 0: outlines filled with palette colours **/
    bool paint = false;   /**< COLR version 1: adds gradients, transforms and composites **/
    bool svg = false;     /**< an SVG document for each glyph **/
    bool cbdt = false;    /**< PNG images at fixed sizes, as Android's emoji are **/
    bool sbix = false;    /**< PNG images at fixed sizes, as Apple's emoji are **/

    /**
     * @return whether there is any colour table
     **/
    bool any() const noexcept;
};

/**
 * What a gradient does past its first and last stops.
 **/
enum class ColourExtend : std::uint8_t {
    Pad,     /**< the end colours carry on **/
    Repeat,  /**< the stops repeat **/
    Reflect  /**< the stops repeat, every other time reversed **/
};

/**
 * How a group is put on what is beneath it, as COLR version 1 names the modes. The first are
 * Porter and Duff's, and the rest blend colours as CSS's blend modes do.
 **/
enum class ColourComposite : std::uint8_t {
    Clear, Source, Destination, SourceOver, DestinationOver, SourceIn, DestinationIn, SourceOut,
    DestinationOut, SourceAtop, DestinationAtop, Xor, Plus,
    Screen, Overlay, Darken, Lighten, ColourDodge, ColourBurn, HardLight, SoftLight, Difference,
    Exclusion, Multiply, Hue, Saturation, Colour, Luminosity
};

/**
 * A closed outline a layer is drawn inside, in em units.
 **/
struct ColourClip final {
    std::vector<QuadraticCurve> curves;
    bool evenOdd = false;  /**< whether it is filled by the even-odd rule rather than the nonzero rule **/
};

/**
 * One colour of a fill or a gradient.
 **/
struct ColourStop final {
    float offset = 0.0F;       /**< its place along a gradient **/
    glm::vec4 colour{ 0.0F };  /**< red, green, blue and alpha, from 0 to 1, as the font gives them **/
    bool foreground = false;   /**< whether its red, green and blue are the text's colour, its alpha times the text's **/
};

/**
 * What fills a layer's clips: one colour, or a gradient.
 *
 * A gradient's points are in its own space, which its transform maps into em units. A linear
 * gradient runs from one point to another. A radial gradient runs from one circle to another,
 * and a sweep gradient around a centre, counter-clockwise from one angle to another.
 **/
struct ColourFill final {
    enum class Kind : std::uint8_t {
        Solid,
        Linear,
        Radial,
        Sweep
    };

    Kind kind = Kind::Solid;
    std::vector<ColourStop> stops;   /**< in order of offset. A solid fill has one **/
    ColourExtend extend = ColourExtend::Pad;
    glm::vec2 from{ 0.0F };          /**< a linear gradient's start, a radial gradient's first centre, or a sweep's centre **/
    glm::vec2 to{ 0.0F };            /**< a linear gradient's end, or a radial gradient's second centre **/
    float fromRadius = 0.0F;
    float toRadius = 0.0F;
    float startAngle = 0.0F;         /**< a sweep's, in radians **/
    float endAngle = 0.0F;
    glm::mat3 transform{ 1.0F };     /**< from the gradient's space into em units **/
};

/**
 * A picture a glyph places, as a bitmap table holds one.
 **/
struct ColourImage final {
    glm::uvec2 size{ 0 };
    std::vector<unsigned char> pixels;  /**< red, green, blue and alpha, unpremultiplied, in rows from the top **/
    glm::vec2 min{ 0.0F };              /**< the lower left of the box it fills, in its own space **/
    glm::vec2 max{ 0.0F };
    glm::mat3 transform{ 1.0F };        /**< from its own space into em units **/
};

/**
 * One step of drawing a colour glyph.
 *
 * A fill or an image is drawn inside every one of its clips. A group gathers the steps between
 * its beginning and its end apart, then is put on what is beneath it by its composite. Steps
 * outside any group, and within one, are drawn over each other in order.
 **/
struct ColourLayer final {
    enum class Kind : std::uint8_t {
        Fill,
        Image,
        BeginGroup,
        EndGroup
    };

    Kind kind = Kind::Fill;
    std::vector<ColourClip> clips;
    ColourFill fill;
    std::size_t image = 0;                           /**< an image's index in its glyph **/
    ColourComposite composite = ColourComposite::SourceOver;  /**< how an ended group is put down **/
    float opacity = 1.0F;                                     /**< what an ended group's colours are multiplied by as it is put down **/
};

/**
 * A colour glyph, as layers of filled outlines and images in em units, with y upwards.
 **/
struct ColourGlyph final {
    std::vector<ColourLayer> layers;
    std::vector<ColourImage> images;
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
ColourGlyph colour(const face::Face& face, unsigned int index, unsigned int palette, float size, const std::vector<std::int32_t>& coordinates = {});

};  // namespace v3d::font::glyph
