/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curve.h"

#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/reyes/Hider.h>
#include <moya/libmoya/reyes/MicroPolygonGrid.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include "RenderContext.h"
#include "Vertex.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya {

namespace {

/**
 * How many times a segment may be halved. A segment measured smaller each time reaches a grid's
 * size long before this, so a segment still not diceable here is one that does not shrink.
 **/
constexpr unsigned int MOST_SPLITS = 16;

/**
 * A cubic's degree, which its derivative is this many times the steps between its points.
 **/
constexpr float DEGREE = 3.0F;

glm::vec3 pointOn(const Curve::Controls & controls, float v) {
    const glm::vec3 first = glm::mix(controls[0], controls[1], v);
    const glm::vec3 middle = glm::mix(controls[1], controls[2], v);
    const glm::vec3 last = glm::mix(controls[2], controls[3], v);
    return glm::mix(glm::mix(first, middle, v), glm::mix(middle, last, v), v);
}

glm::vec3 tangentOn(const Curve::Controls & controls, float v) {
    const glm::vec3 first = glm::mix(controls[1] - controls[0], controls[2] - controls[1], v);
    const glm::vec3 last = glm::mix(controls[2] - controls[1], controls[3] - controls[2], v);
    return DEGREE * glm::mix(first, last, v);
}

/**
 * Any direction square to another, for a curve seen end on.
 **/
glm::vec3 square(const glm::vec3 & direction) {
    const glm::vec3 axis = std::abs(direction.x) < std::abs(direction.y) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    return glm::normalize(glm::cross(direction, axis));
}

};  // namespace

Curve::Curve(const Controls & controls, const glm::vec2 & widths) : controls_(controls), widths_(widths) {
}

const Curve::Controls & Curve::controls() const {
    return controls_;
}

const glm::vec2 & Curve::widths() const {
    return widths_;
}

void Curve::toEye() {
    const glm::mat4 & toEye = placement();
    for (glm::vec3 & control : controls_) {
        control = glm::vec3(toEye * glm::vec4(control, 1.0F));
    }
    widths_ *= widthScale(toEye);
}

v3d::type::geometry::AABBox Curve::hull(void) const {
    // the curve lies inside its control points' hull
    v3d::type::geometry::AABBox box;
    box.extents(glm::min(glm::min(controls_[0], controls_[1]), glm::min(controls_[2], controls_[3])),
        glm::max(glm::max(controls_[0], controls_[1]), glm::max(controls_[2], controls_[3])));
    return box;
}

v3d::type::geometry::AABBox Curve::bound(void) const {
    // the ribbon reaches half its width beyond the curve
    const float reach = 0.5F * std::max(widths_.x, widths_.y);
    const v3d::type::geometry::AABBox centre = hull();
    v3d::type::geometry::AABBox box;
    box.extents(centre.min() - glm::vec3(reach), centre.max() + glm::vec3(reach));
    return box;
}

void Curve::split(reyes::Hider & hider, RenderContext & rc) {
    if (splits_ >= MOST_SPLITS) {
        return;
    }
    // de Casteljau's construction at the middle gives the two halves, each a Bezier segment
    const glm::vec3 first = glm::mix(controls_[0], controls_[1], 0.5F);
    const glm::vec3 middle = glm::mix(controls_[1], controls_[2], 0.5F);
    const glm::vec3 last = glm::mix(controls_[2], controls_[3], 0.5F);
    const glm::vec3 left = glm::mix(first, middle, 0.5F);
    const glm::vec3 right = glm::mix(middle, last, 0.5F);
    const glm::vec3 centre = glm::mix(left, right, 0.5F);
    const float width = 0.5F * (widths_.x + widths_.y);
    for (const auto& [controls, widths] : { std::pair(Controls{ controls_[0], first, left, centre }, glm::vec2(widths_.x, width)),
        std::pair(Controls{ centre, right, last, controls_[3] }, glm::vec2(width, widths_.y)) }) {
        const boost::shared_ptr<Curve> piece = boost::make_shared<Curve>(controls, widths);
        piece->place(placement(), color(), normal(), shading());
        piece->motion(motion());
        piece->splits_ = splits_ + 1;
        hider.add(piece, &rc);
    }
}

bool Curve::dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, RenderContext & rc) {
    // reyes::Hider::add() splits anything wider or taller than one grid, so one grid covers
    // what reaches here
    const auto across = static_cast<unsigned int>(std::sqrt(static_cast<float>(rc.gridSize())));  // checked: the root of a count is finite and no larger
    return dice(grid, across);
}

bool Curve::dice(boost::shared_ptr<reyes::MicroPolygonGrid> & grid, unsigned int across) {
    if (diced_ || across == 0) {
        return false;
    }
    diced_ = true;
    grid.reset(new reyes::MicroPolygonGrid(across + 1));
    const unsigned int size = grid->size();
    const float span = static_cast<float>(size - 1);
    for (unsigned int i = 0; i < size; i++) {
        const float v = static_cast<float>(i) / span;
        const glm::vec3 centre = pointOn(controls_, v);
        const glm::vec3 tangent = tangentOn(controls_, v);
        // the eye is at the origin, so the ribbon spreads square both to the curve and to the
        // line of sight, and its normal turns toward the eye
        glm::vec3 side = glm::cross(tangent, centre);
        const float sideLength = glm::length(side);
        side = sideLength > 0.0F ? side / sideLength : square(glm::length(tangent) > 0.0F ? tangent : centre);
        glm::vec3 facing = glm::cross(side, tangent);
        const float facingLength = glm::length(facing);
        facing = facingLength > 0.0F ? facing / facingLength : -glm::normalize(centre);
        if (glm::dot(facing, centre) > 0.0F) {
            facing = -facing;
        }
        const float width = glm::mix(widths_.x, widths_.y, v);
        for (unsigned int j = 0; j < size; j++) {
            const float u = static_cast<float>(j) / span;
            Vertex vert;
            vert.point(centre + (side * (width * (u - 0.5F))));
            vert.color(color());
            vert.normal(facing);
            vert.geometricNormal(facing);
            vert.st(glm::vec2(u, v));
            grid->addVertex(vert, i, j);
        }
    }
    return true;
}

float widthScale(const glm::mat4 & transform) {
    return std::cbrt(std::abs(glm::determinant(glm::mat3(transform))));
}

};  // namespace v3d::moya
