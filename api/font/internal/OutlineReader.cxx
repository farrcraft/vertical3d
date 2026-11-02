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

#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::internal {

OutlineReader::OutlineReader(float unitsPerEm) :
    scale_(unitsPerEm > 0.0F ? 1.0F / unitsPerEm : 0.0F) {
}

std::vector<QuadraticCurve> OutlineReader::read(FT_Outline* outline) {
    curves_.clear();
    const FT_Outline_Funcs funcs{ .move_to = moveTo, .line_to = lineTo, .conic_to = conicTo, .cubic_to = cubicTo, .shift = 0, .delta = 0 };
    if (FT_Outline_Decompose(outline, &funcs, this) != 0) {
        curves_.clear();
    }
    return curves_;
}

int OutlineReader::moveTo(const FT_Vector* to, void* user) {
    OutlineReader* reader = static_cast<OutlineReader*>(user);
    reader->pen_ = reader->at(to);
    return 0;
}

int OutlineReader::lineTo(const FT_Vector* to, void* user) {
    OutlineReader* reader = static_cast<OutlineReader*>(user);
    const glm::vec2 end = reader->at(to);
    reader->add(end, end);
    return 0;
}

int OutlineReader::conicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
    OutlineReader* reader = static_cast<OutlineReader*>(user);
    reader->add(reader->at(control), reader->at(to));
    return 0;
}

int OutlineReader::cubicTo(const FT_Vector* first, const FT_Vector* second, const FT_Vector* to, void* user) {
    OutlineReader* reader = static_cast<OutlineReader*>(user);
    glm::vec2 start = reader->pen_;
    glm::vec2 near = reader->at(first);
    glm::vec2 far = reader->at(second);
    const glm::vec2 end = reader->at(to);
    for (int left = CUBIC_PIECES; left > 0; left--) {
        // split off the next piece where it ends, so each piece is the cubic itself over its
        // stretch, and the rest carries on from there
        const float at = 1.0F / static_cast<float>(left);
        const glm::vec2 firstLeg = glm::mix(start, near, at);
        const glm::vec2 middleLeg = glm::mix(near, far, at);
        const glm::vec2 lastLeg = glm::mix(far, end, at);
        const glm::vec2 pieceFar = glm::mix(firstLeg, middleLeg, at);
        const glm::vec2 restNear = glm::mix(middleLeg, lastLeg, at);
        const glm::vec2 split = glm::mix(pieceFar, restNear, at);
        // the piece runs from start through firstLeg and pieceFar to split. A quadratic's control
        // is where each of those two controls would put it, extended from its end by half again,
        // and is taken halfway between the two
        const glm::vec2 fromStart = glm::mix(start, firstLeg, EXTENDED);
        const glm::vec2 fromEnd = glm::mix(split, pieceFar, EXTENDED);
        reader->add(glm::mix(fromStart, fromEnd, HALFWAY), split);
        start = split;
        near = restNear;
        far = lastLeg;
    }
    return 0;
}

glm::vec2 OutlineReader::at(const FT_Vector* point) const {
    return glm::vec2(static_cast<float>(point->x), static_cast<float>(point->y)) * scale_;
}

void OutlineReader::add(const glm::vec2& control, const glm::vec2& to) {
    if (to != pen_ || control != pen_) {
        curves_.push_back(QuadraticCurve{ .p1 = pen_, .p2 = control, .p3 = to });
    }
    pen_ = to;
}

};  // namespace v3d::font::internal
