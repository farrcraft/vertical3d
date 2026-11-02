/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/Curves.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <vector>

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
     * @return the outline's curves, or none when it cannot be read
     **/
    std::vector<QuadraticCurve> read(FT_Outline* outline);

 private:
    /**
     * How many quadratics a cubic is split into.
     **/
    static constexpr int CUBIC_PIECES = 4;

    /**
     * How far past a cubic's control a quadratic's reaches from the same end, as a share of the
     * way from the end to the control.
     **/
    static constexpr float EXTENDED = 1.5F;

    /**
     * Halfway between two points.
     **/
    static constexpr float HALFWAY = 0.5F;

    static int moveTo(const FT_Vector* to, void* user);
    static int lineTo(const FT_Vector* to, void* user);
    static int conicTo(const FT_Vector* control, const FT_Vector* to, void* user);
    static int cubicTo(const FT_Vector* first, const FT_Vector* second, const FT_Vector* to, void* user);

    /**
     * @return a point in em units
     **/
    glm::vec2 at(const FT_Vector* point) const;

    /**
     * Add a curve from the pen, and move the pen to its end. A curve that goes nowhere is left out.
     **/
    void add(const glm::vec2& control, const glm::vec2& to);

    float scale_;
    glm::vec2 pen_{ 0.0F };
    std::vector<QuadraticCurve> curves_;
};

};  // namespace v3d::font::internal
