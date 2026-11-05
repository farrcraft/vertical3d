/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveBuilder.h"

#include <api/font/glyph/Curves.h>

#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::font::internal {

CurveBuilder::CurveBuilder(const glm::mat3& transform) :
    transform_(transform) {
}

void CurveBuilder::moveTo(const glm::vec2& to) {
    close();
    pen_ = map(to);
    start_ = pen_;
}

void CurveBuilder::lineTo(const glm::vec2& to) {
    const glm::vec2 end = map(to);
    add(end, end);
}

void CurveBuilder::quadraticTo(const glm::vec2& control, const glm::vec2& to) {
    add(map(control), map(to));
}

void CurveBuilder::cubicTo(const glm::vec2& first, const glm::vec2& second, const glm::vec2& to) {
    glm::vec2 start = pen_;
    glm::vec2 near = map(first);
    glm::vec2 far = map(second);
    const glm::vec2 end = map(to);
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
        add(glm::mix(fromStart, fromEnd, HALFWAY), split);
        start = split;
        near = restNear;
        far = lastLeg;
    }
}

void CurveBuilder::close() {
    if (pen_ != start_) {
        add(start_, start_);
    }
}

std::vector<glyph::QuadraticCurve> CurveBuilder::take() {
    close();
    return std::exchange(curves_, std::vector<glyph::QuadraticCurve>());
}

glm::vec2 CurveBuilder::map(const glm::vec2& point) const {
    return glm::vec2(transform_ * glm::vec3(point, 1.0F));
}

void CurveBuilder::add(const glm::vec2& control, const glm::vec2& to) {
    if (to != pen_ || control != pen_) {
        curves_.push_back(glyph::QuadraticCurve{ .p1 = pen_, .p2 = control, .p3 = to });
    }
    pen_ = to;
}

};  // namespace v3d::font::internal
