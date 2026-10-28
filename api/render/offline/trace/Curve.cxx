/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curve.h"

#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Ray.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>

#include "Hit.h"
#include "Intersection.h"
#include "Pose.h"
#include "Primitive.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::offline::trace {

namespace {

/**
 * The most times a segment is halved before its pieces are tested as straight lines. A
 * thousand pieces are straight enough for any curve a scene draws.
 **/
constexpr int DEEPEST = 10;

/**
 * How far a piece may bow from straight, as a fraction of the ribbon's width, before it is
 * halved again.
 **/
constexpr float BOW = 0.05F;

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
 * The two halves of a segment, by de Casteljau's construction at its middle.
 **/
std::pair<Curve::Controls, Curve::Controls> halves(const Curve::Controls & controls) {
    const glm::vec3 first = glm::mix(controls[0], controls[1], 0.5F);
    const glm::vec3 middle = glm::mix(controls[1], controls[2], 0.5F);
    const glm::vec3 last = glm::mix(controls[2], controls[3], 0.5F);
    const glm::vec3 left = glm::mix(first, middle, 0.5F);
    const glm::vec3 right = glm::mix(middle, last, 0.5F);
    const glm::vec3 centre = glm::mix(left, right, 0.5F);
    return { { controls[0], first, left, centre }, { centre, right, last, controls[3] } };
}

/**
 * The ray in the space the segment is stored in: where it starts, and its unit direction.
 **/
struct Local {
    glm::vec3 origin;
    glm::vec3 along;
};

/**
 * A search of one segment, in a frame where the ray runs along +z from the origin.
 **/
class Search final {
 public:
    Search(const glm::vec2 & widths, float from, const Pose & pose, const Local & local, const v3d::type::geometry::Ray & ray) :
        widths_(widths), from_(from), pose_(pose), origin_(local.origin), along_(local.along), ray_(ray) {
    }

    /**
     * Halve a piece until it is nearly straight, keeping the nearest hit on any piece.
     **/
    void pieces(const Curve::Controls & piece, int depth, float start, float end) {
        const float reach = 0.5F * std::max(width(start), width(end));
        glm::vec3 lowest = glm::min(glm::min(piece[0], piece[1]), glm::min(piece[2], piece[3]));
        glm::vec3 highest = glm::max(glm::max(piece[0], piece[1]), glm::max(piece[2], piece[3]));
        lowest -= glm::vec3(reach);
        highest += glm::vec3(reach);
        // the ray is the z axis, so a piece whose box does not hold it, or lies wholly behind
        // the origin, is missed
        if (lowest.x > 0.0F || highest.x < 0.0F || lowest.y > 0.0F || highest.y < 0.0F || highest.z < 0.0F) {
            return;
        }
        if (depth > 0) {
            const auto [left, right] = halves(piece);
            const float middle = 0.5F * (start + end);
            pieces(left, depth - 1, start, middle);
            pieces(right, depth - 1, middle, end);
            return;
        }
        straight(piece, start, end);
    }

    bool found() const {
        return found_;
    }

    const Intersection & nearest() const {
        return nearest_;
    }

 private:
    float width(float v) const {
        return glm::mix(widths_.x, widths_.y, v);
    }

    /**
     * Test a piece taken to be straight: whether the ray passes between its two ends, and
     * within half the ribbon's width of the curve there.
     **/
    void straight(const Curve::Controls & piece, float start, float end) {
        // the ray has to be past the plane square to the piece at its start, and short of the
        // one at its end
        const glm::vec2 first(piece[0]);
        const glm::vec2 last(piece[3]);
        if (glm::dot(-first, glm::vec2(piece[1]) - first) < 0.0F || glm::dot(-last, glm::vec2(piece[2]) - last) < 0.0F) {
            return;
        }
        const glm::vec2 chord = last - first;
        const float chordLength = glm::dot(chord, chord);
        if (!(chordLength > 0.0F)) {
            return;
        }
        const float w = std::clamp(glm::dot(-first, chord) / chordLength, 0.0F, 1.0F);
        const glm::vec3 centre = pointOn(piece, w);
        const float v = glm::mix(start, end, w);
        const float half = 0.5F * width(v);
        const float offset = glm::length(glm::vec2(centre));
        if (offset > half || centre.z <= 0.0F) {
            return;
        }
        const std::optional<float> distance = along(centre.z);
        if (!distance || *distance <= from_ || (found_ && *distance >= nearest_.distance)) {
            return;
        }
        // which side of the centre line the ray passes on gives how far across the ribbon it is
        const glm::vec2 tangent(tangentOn(piece, w));
        const float side = (tangent.x * -centre.y) - (tangent.y * -centre.x);
        found_ = true;
        nearest_.distance = *distance;
        nearest_.v = v;
        nearest_.u = half > 0.0F ? 0.5F + (std::copysign(offset, side) / (2.0F * half)) : 0.5F;
    }

    /**
     * A distance along the local ray as a distance along the ray the scene cast, which a
     * motion that scales does not keep.
     **/
    std::optional<float> along(float local) const {
        if (pose_.ahead == nullptr) {
            return local;
        }
        const glm::vec3 there(*pose_.ahead * glm::vec4(origin_ + (along_ * local), 1.0F));
        return glm::dot(there - ray_.origin(), ray_.direction());
    }

    glm::vec2 widths_;
    float from_;
    const Pose & pose_;
    glm::vec3 origin_;
    glm::vec3 along_;
    const v3d::type::geometry::Ray & ray_;
    bool found_ = false;
    Intersection nearest_;
};

};  // namespace

Curve::Curve(const Controls & controls, const glm::vec2 & widths, const glm::vec3 & colour) :
    Primitive(colour), controls_(controls), widths_(widths) {
}

const Curve::Controls & Curve::controls() const {
    return controls_;
}

const glm::vec2 & Curve::widths() const {
    return widths_;
}

glm::vec3 Curve::point(float v) const {
    return pointOn(controls_, v);
}

bool Curve::intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
    Intersection* found) const {
    glm::vec3 origin = ray.origin();
    glm::vec3 direction = ray.direction();
    if (pose.backward != nullptr) {
        origin = glm::vec3(*pose.backward * glm::vec4(origin, 1.0F));
        direction = glm::mat3(*pose.backward) * direction;
    }
    const float length = glm::length(direction);
    const float widest = std::max(widths_.x, widths_.y);
    if (!(length > 0.0F) || !(widest > 0.0F)) {
        return false;
    }
    const glm::vec3 along = direction / length;

    // a frame with the ray along +z from the origin. Any two directions square to it will do,
    // since the ribbon turns to face the ray
    const glm::vec3 side = glm::normalize(std::abs(along.x) > std::abs(along.y) ?
        glm::vec3(-along.z, 0.0F, along.x) : glm::vec3(0.0F, along.z, -along.y));
    const glm::vec3 up = glm::cross(along, side);
    Controls local{};
    for (std::size_t each = 0; each < local.size(); each++) {
        const glm::vec3 offset = controls_[each] - origin;
        local[each] = glm::vec3(glm::dot(offset, side), glm::dot(offset, up), glm::dot(offset, along));
    }

    // halve the segment until each piece bows from straight by less than a twentieth of the
    // ribbon's width. Each halving quarters how far a piece bows
    float bend = 0.0F;
    for (std::size_t each = 0; each + 2 < local.size(); each++) {
        const glm::vec3 second = glm::abs(local[each] - (2.0F * local[each + 1]) + local[each + 2]);
        bend = std::max(bend, std::max(second.x, std::max(second.y, second.z)));
    }
    int depth = 0;
    const float tolerance = BOW * widest;
    if (bend > 0.0F) {
        const float levels = std::log2(std::sqrt(2.0F) * 6.0F * bend / (8.0F * tolerance)) / 2.0F;
        // a NaN fails the test, and std::min bounds an infinity
        depth = levels > 0.0F ? static_cast<int>(std::lround(std::min(levels, static_cast<float>(DEEPEST)))) : 0;  // checked: levels is in (0, DEEPEST]
    }

    Search search(widths_, from, pose, { .origin = origin, .along = along }, ray);
    search.pieces(local, depth, 0.0F, 1.0F);
    if (!search.found()) {
        return false;
    }
    *found = search.nearest();
    // the ribbon faces back along the ray, turned square to the curve
    const glm::vec3 tangent = tangentOn(controls_, found->v);
    const float tangentLength = glm::length(tangent);
    glm::vec3 facing = -along;
    if (tangentLength > 0.0F) {
        const glm::vec3 unit = tangent / tangentLength;
        const glm::vec3 square = facing - (glm::dot(facing, unit) * unit);
        const float squareLength = glm::length(square);
        if (squareLength > 0.0F) {
            facing = square / squareLength;
        }
    }
    found->normal = facing;
    return true;
}

void Curve::describe(const Intersection & found, Hit* hit) const {
    hit->normal = found.normal;
    hit->geometric = found.normal;
    hit->u = found.u;
    hit->v = found.v;
    hit->s = found.u;
    hit->t = found.v;
    hit->colour = colour();
}

bool Curve::bound(v3d::type::geometry::AABBox* box) const {
    // the curve lies inside its control points' hull, and the ribbon reaches half its width
    // beyond the curve
    const float reach = 0.5F * std::max(widths_.x, widths_.y);
    const glm::vec3 lowest = glm::min(glm::min(controls_[0], controls_[1]), glm::min(controls_[2], controls_[3]));
    const glm::vec3 highest = glm::max(glm::max(controls_[0], controls_[1]), glm::max(controls_[2], controls_[3]));
    box->extents(lowest - glm::vec3(reach), highest + glm::vec3(reach));
    return true;
}

};  // namespace v3d::render::offline::trace
