/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/glyph/Curves.h>
#include <api/font/internal/CurveBuilder.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::internal {

/**
 * Turns a FreeType outline into quadratic curves in em units, through FT_Outline_Decompose.
 **/
class OutlineReader final {
 public:
    /**
     * @param unitsPerEm the face's design units to the em, which every point is divided by
     **/
    explicit OutlineReader(float unitsPerEm);

    /**
     * @param transform maps every point, in the outline's units, into the curves' space
     **/
    explicit OutlineReader(const glm::mat3& transform);

    /**
     * @return the outline's curves, or none when it cannot be read
     **/
    std::vector<glyph::QuadraticCurve> read(FT_Outline* outline);

 private:
    static int moveTo(const FT_Vector* to, void* user);
    static int lineTo(const FT_Vector* to, void* user);
    static int conicTo(const FT_Vector* control, const FT_Vector* to, void* user);
    static int cubicTo(const FT_Vector* first, const FT_Vector* second, const FT_Vector* to, void* user);

    /**
     * @return a point in design units
     **/
    static glm::vec2 at(const FT_Vector* point);

    CurveBuilder builder_;
};

};  // namespace v3d::font::internal
