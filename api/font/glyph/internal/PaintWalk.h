/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/glyph/ColourGlyph.h>
#include <api/font/internal/CurveBuilder.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <optional>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::internal {
class SvgDocuments;
};  // namespace v3d::font::internal

namespace v3d::font::glyph::internal {

/**
 * Everything a walk through a glyph's paint has seen so far.
 *
 * HarfBuzz gives every coordinate in the font's scale, under the transforms pushed so far.
 * Outlines are mapped into em units as they are read, and a gradient keeps its own coordinates
 * with the transform that maps them there.
 **/
struct PaintWalk final {
    /**
     * A box grown to hold points, empty until it holds one.
     **/
    struct Box final {
        glm::vec2 min{ 0.0F };
        glm::vec2 max{ 0.0F };
        bool any = false;

        void hold(const glm::vec2& point);
    };

    FT_Library library = nullptr;
    FT_Memory memory = nullptr;           /**< the face's, which inflates a compressed document **/
    font::internal::SvgDocuments* documents = nullptr;    /**< the face's SVG documents, parsed once each **/
    unsigned int index = 0;      /**< the glyph walked **/
    unsigned int units = 0;      /**< the face's design units to the em **/
    float finer = 1.0F;          /**< the font's scale in design units **/
    glm::mat3 toEm{ 1.0F };
    std::vector<glm::mat3> transforms{ glm::mat3(1.0F) };
    std::vector<ColourClip> clips;
    std::optional<font::internal::CurveBuilder> path;
    ColourGlyph glyph;

    /**
     * @return the transform from the font's scale, as transformed now, into em units
     **/
    glm::mat3 current() const;

    /**
     * Add a fill inside the clips pushed so far.
     **/
    void fill(ColourFill&& made);
};

};  // namespace v3d::font::glyph::internal
