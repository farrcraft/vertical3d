/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "OutlineReader.h"

#include <api/font/Curves.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::internal {

namespace {

/**
 * @return the transform that divides a point in design units by the units to the em
 **/
glm::mat3 toEm(float unitsPerEm) {
    const float scale = unitsPerEm > 0.0F ? 1.0F / unitsPerEm : 0.0F;
    glm::mat3 scaling(scale);
    scaling[2][2] = 1.0F;
    return scaling;
}

};  // namespace

OutlineReader::OutlineReader(float unitsPerEm) :
    builder_(toEm(unitsPerEm)) {
}

std::vector<QuadraticCurve> OutlineReader::read(FT_Outline* outline) {
    const FT_Outline_Funcs funcs{ .move_to = moveTo, .line_to = lineTo, .conic_to = conicTo, .cubic_to = cubicTo, .shift = 0, .delta = 0 };
    if (FT_Outline_Decompose(outline, &funcs, this) != 0) {
        builder_.take();
        return std::vector<QuadraticCurve>();
    }
    return builder_.take();
}

int OutlineReader::moveTo(const FT_Vector* to, void* user) {
    static_cast<OutlineReader*>(user)->builder_.moveTo(at(to));
    return 0;
}

int OutlineReader::lineTo(const FT_Vector* to, void* user) {
    static_cast<OutlineReader*>(user)->builder_.lineTo(at(to));
    return 0;
}

int OutlineReader::conicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
    static_cast<OutlineReader*>(user)->builder_.quadraticTo(at(control), at(to));
    return 0;
}

int OutlineReader::cubicTo(const FT_Vector* first, const FT_Vector* second, const FT_Vector* to, void* user) {
    static_cast<OutlineReader*>(user)->builder_.cubicTo(at(first), at(second), at(to));
    return 0;
}

glm::vec2 OutlineReader::at(const FT_Vector* point) {
    return glm::vec2(static_cast<float>(point->x), static_cast<float>(point->y));
}

};  // namespace v3d::font::internal
