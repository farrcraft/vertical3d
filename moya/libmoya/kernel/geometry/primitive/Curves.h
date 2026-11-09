/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/primitive/Quadratic.h>
#include <moya/libmoya/kernel/geometry/primitive/RayFrame.h>
#include <moya/libmoya/kernel/geometry/primitive/ShapeHit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/**
 * The segments of curves, and the tests that find where a ray meets one. A segment is a list of
 * control points, each a centre in x, y and z and a radius in w. A linear segment has two, and a
 * cubic one four, as a Bézier curve's.
 *
 * A round segment is the surface swept by a sphere of the curve's radius moving along it. A flat
 * segment faces the ray: the ray meets it where it passes within the radius of a point on the
 * centre line, as the ray sees the curve. The hit is at the depth of the point it passes deepest
 * inside, by the square of the distance from the point less the square of its radius. A ray meets
 * a flat segment exactly where it would meet the round one.
 *
 * Two segments of a curve share the point where one ends and the next starts. The next segment
 * holds it, so a ray that meets it there meets one segment and not both. A segment that closes
 * its curve holds its end too.
 *
 * A hit's position is across the width in u, from 0 to 1 with a half on the centre line, and
 * along the segment in v, from 0 to 1.
 **/
namespace v3d::moya::kernel::geometry::primitive {

/** How many times a cubic segment is halved before a hit on a piece of it is refined. **/
inline constexpr std::uint32_t kCurveDepth = 4;

/** How many steps Newton's method takes to refine a hit. **/
inline constexpr std::uint32_t kCurveSteps = 8;

/** How far off a round curve's surface a refined hit may lie, as a share of the radius. **/
inline constexpr float kCurveTolerance = 1.0e-3F;

/** How much more it may lie off, as a share of the size of the coordinates, for their rounding. **/
inline constexpr float kCurveRounding = 1.0e-6F;

/** A cubic's degree, of which a Bézier segment's weights and derivatives are multiples. **/
inline constexpr float kCubic = 3.0F;

/** How many control points a segment of a form holds: two linear, four cubic, else none. **/
MOYA_KERNEL inline std::uint32_t controlsOf(Form form) {
    switch (form) {
        case Form::RoundLinearCurves:
        case Form::FlatLinearCurves:
            return 2;
        case Form::RoundCubicCurves:
        case Form::FlatCubicCurves:
            return 4;
        case Form::Triangles:
        case Form::RoundPoints:
        case Form::FlatPoints:
        case Form::Quadric:
            break;
    }
    return 0;
}

/** A cubic Bézier segment's centre and radius at a position along it. **/
MOYA_KERNEL inline glm::vec4 bezierAt(const glm::vec4 * controls, float along) {
    const float before = 1.0F - along;
    return (controls[0] * (before * before * before)) + (controls[1] * (kCubic * before * before * along)) +
        (controls[2] * (kCubic * before * along * along)) + (controls[3] * (along * along * along));
}

/** The first derivative of a cubic Bézier segment by its position. **/
MOYA_KERNEL inline glm::vec4 bezierSlope(const glm::vec4 * controls, float along) {
    const float before = 1.0F - along;
    return ((controls[1] - controls[0]) * (kCubic * before * before)) + ((controls[2] - controls[1]) * (2.0F * kCubic * before * along)) +
        ((controls[3] - controls[2]) * (kCubic * along * along));
}

/** The second derivative of a cubic Bézier segment by its position. **/
MOYA_KERNEL inline glm::vec4 bezierBend(const glm::vec4 * controls, float along) {
    return ((controls[2] - (controls[1] * 2.0F) + controls[0]) * (2.0F * kCubic * (1.0F - along))) +
        ((controls[3] - (controls[2] * 2.0F) + controls[1]) * (2.0F * kCubic * along));
}

/** A cubic Bézier segment cut in half at its middle, as the control points of each half. **/
MOYA_KERNEL inline void halve(const glm::vec4 * controls, glm::vec4 * first, glm::vec4 * second) {
    const glm::vec4 left = (controls[0] + controls[1]) * 0.5F;
    const glm::vec4 middle = (controls[1] + controls[2]) * 0.5F;
    const glm::vec4 right = (controls[2] + controls[3]) * 0.5F;
    const glm::vec4 leftMiddle = (left + middle) * 0.5F;
    const glm::vec4 rightMiddle = (middle + right) * 0.5F;
    const glm::vec4 centre = (leftMiddle + rightMiddle) * 0.5F;
    first[0] = controls[0];
    first[1] = left;
    first[2] = leftMiddle;
    first[3] = centre;
    second[0] = centre;
    second[1] = rightMiddle;
    second[2] = right;
    second[3] = controls[3];
}

/**
 * Where a ray meets a curve across its width, from 0 to 1: a half on the centre line, and more to
 * the left of the curve's direction as the ray sees it. Takes the centre line's point and
 * direction at the hit, in the ray's frame.
 **/
MOYA_KERNEL inline float acrossOf(const glm::vec4 & centre, const glm::vec4 & slope) {
    const float run = std::sqrt((slope.x * slope.x) + (slope.y * slope.y));
    if (!(run > 0.0F) || !(centre.w > 0.0F)) {
        return 0.5F;
    }
    // the ray lies at minus the centre's offset from the centre line
    const float side = ((slope.y * centre.x) - (slope.x * centre.y)) / run;
    return std::fmin(1.0F, std::fmax(0.0F, 0.5F + (0.5F * side / centre.w)));
}

/**
 * The distance in [start, end) at which a ray, in its own frame, crosses a sphere of a centre and
 * radius, the nearer crossing first, or false.
 **/
MOYA_KERNEL inline bool sphereInFrame(const glm::vec4 & sphere, float length, float start, float end, float & distance) {
    const float gap = (sphere.w * sphere.w) - ((sphere.x * sphere.x) + (sphere.y * sphere.y));
    if (!(gap >= 0.0F)) {
        return false;
    }
    const float half = std::sqrt(gap);
    const float near = (sphere.z - half) / length;
    distance = near >= start ? near : (sphere.z + half) / length;
    return distance >= start && distance < end;
}

/**
 * Whether a ray, in its own frame, may meet a piece of a cubic segment within [start, end): whether
 * it passes through the box of the piece's control points grown by their largest radius. A piece
 * lies inside the hull of its control points, and its radius never exceeds theirs.
 **/
MOYA_KERNEL inline bool mayMeet(const glm::vec4 * piece, float length, float start, float end) {
    glm::vec4 lower = piece[0];
    glm::vec4 upper = piece[0];
    for (std::uint32_t at = 1; at < 4; ++at) {
        lower = glm::min(lower, piece[at]);
        upper = glm::max(upper, piece[at]);
    }
    const float radius = upper.w;
    return lower.x - radius <= 0.0F && upper.x + radius >= 0.0F && lower.y - radius <= 0.0F && upper.y + radius >= 0.0F &&
        lower.z - radius < end * length && upper.z + radius >= start * length;
}

/**
 * A guess at where a ray, in its own frame, meets a piece of a cubic segment, which covers a range
 * of positions along it. The guess is the position where the ray passes nearest the piece's chord,
 * and the distance at which it reaches the front of that point's sphere.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the piece's range, then the guesses
MOYA_KERNEL inline void guessNearest(const glm::vec4 * piece, float low, float width, float length, float & along, float & distance) {
    const glm::vec4 chord = piece[3] - piece[0];
    const float run = (chord.x * chord.x) + (chord.y * chord.y);
    const float share = run > 0.0F ? std::fmin(1.0F, std::fmax(0.0F, -((piece[0].x * chord.x) + (piece[0].y * chord.y)) / run)) : 0.5F;
    const glm::vec4 nearest = piece[0] + (chord * share);
    along = low + (share * width);
    distance = (nearest.z - nearest.w) / length;
}

/**
 * Refines where a ray, in its own frame, meets a round cubic segment, by Newton's method from a
 * guess at the position along it and the distance. A hit on the swept surface lies at the radius
 * from the centre line, and where the sphere at its position touches the surface. Gives false when
 * the method does not settle on the surface within the segment.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the position, then the distance
MOYA_KERNEL inline bool refineRound(const glm::vec4 * controls, float length, float & along, float & distance) {
    float position = along;
    float reach = distance;
    for (std::uint32_t step = 0; step < kCurveSteps; ++step) {
        const glm::vec4 centre = bezierAt(controls, position);
        const glm::vec4 slope = bezierSlope(controls, position);
        const glm::vec4 bend = bezierBend(controls, position);
        const glm::vec3 offset { -centre.x, -centre.y, (reach * length) - centre.z };
        const glm::vec3 tangent(slope);
        // touch is zero where the sphere touches the surface, and gap where the ray is on it
        const float touch = glm::dot(offset, tangent) + (centre.w * slope.w);
        const float gap = glm::dot(offset, offset) - (centre.w * centre.w);
        const float touchByPosition = -glm::dot(tangent, tangent) + glm::dot(offset, glm::vec3(bend)) + (slope.w * slope.w) + (centre.w * bend.w);
        const float touchByDistance = length * tangent.z;
        const float gapByPosition = -2.0F * touch;
        const float gapByDistance = 2.0F * length * offset.z;
        const float determinant = (touchByPosition * gapByDistance) - (touchByDistance * gapByPosition);
        if (!(std::fabs(determinant) > 0.0F)) {
            return false;
        }
        position -= ((touch * gapByDistance) - (touchByDistance * gap)) / determinant;
        reach -= ((touchByPosition * gap) - (gapByPosition * touch)) / determinant;
        if (!(position >= -1.0F) || !(position <= 2.0F)) {
            return false;
        }
    }
    if (!(position >= 0.0F) || !(position <= 1.0F)) {
        return false;
    }
    const glm::vec4 centre = bezierAt(controls, position);
    const glm::vec4 slope = bezierSlope(controls, position);
    const glm::vec3 tangent(slope);
    const glm::vec3 offset { -centre.x, -centre.y, (reach * length) - centre.z };
    const float size = std::fabs(centre.x) + std::fabs(centre.y) + std::fabs(centre.z) + std::fabs(reach * length);
    const float allowed = (kCurveTolerance * centre.w) + (kCurveRounding * size);
    if (std::fabs(glm::length(offset) - centre.w) > allowed || std::fabs(glm::dot(offset, tangent) + (centre.w * slope.w)) > allowed * glm::length(tangent)) {
        return false;
    }
    along = position;
    distance = reach;
    return true;
}

/**
 * The position along a flat cubic segment that a ray passes deepest inside, as the ray sees it in
 * its own frame. It is where the square of the distance from the centre line, less the square of
 * the radius, is least. It is refined by Newton's method from a guess and held to the segment.
 * Where that does not curve upward, the position reached stands.
 **/
MOYA_KERNEL inline float refineFlat(const glm::vec4 * controls, float along) {
    float position = along;
    for (std::uint32_t step = 0; step < kCurveSteps; ++step) {
        const glm::vec4 centre = bezierAt(controls, position);
        const glm::vec4 slope = bezierSlope(controls, position);
        const glm::vec4 bend = bezierBend(controls, position);
        const float lean = (centre.x * slope.x) + (centre.y * slope.y) - (centre.w * slope.w);
        const float turn = (slope.x * slope.x) + (slope.y * slope.y) + (centre.x * bend.x) + (centre.y * bend.y) - (slope.w * slope.w) - (centre.w * bend.w);
        if (!(turn > 0.0F)) {
            break;
        }
        position = std::fmin(1.0F, std::fmax(0.0F, position - (lean / turn)));
    }
    return position;
}

/**
 * Whether a ray, in its own frame, passes within a flat curve's radius of a point on its centre
 * line, at the point's depth within [start, end), and where.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the interval
MOYA_KERNEL inline bool flatAt(const glm::vec4 & centre, const glm::vec4 & slope, float along, float length, float start, float end, ShapeHit & hit) {
    const float distance = centre.z / length;
    if ((centre.x * centre.x) + (centre.y * centre.y) > centre.w * centre.w || distance < start || !(distance < end)) {
        return false;
    }
    hit.distance = distance;
    hit.uv = glm::vec2 { acrossOf(centre, slope), along };
    return true;
}

/**
 * Whether a ray, in its own frame, meets a round linear segment within [start, end), and where.
 * The segment is a sphere at each end and the cone that touches both. A point on the cone lies
 * where the sphere at its position touches it, which ties the position to the distance linearly,
 * so the cone's crossings are the roots of a quadratic.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - whether it closes, then the interval
MOYA_KERNEL inline bool intersectRoundLinear(const glm::vec4 * controls, bool closes, float length, float start, float end, ShapeHit & hit) {
    const glm::vec4 & from = controls[0];
    const glm::vec4 slope = controls[1] - from;
    bool found = false;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the position
    const auto keep = [&](float distance, float along) {
        end = distance;
        hit.distance = distance;
        hit.uv = glm::vec2 { acrossOf(from + (slope * along), slope), along };
        found = true;
    };
    float distance = 0.0F;
    if (sphereInFrame(from, length, start, end, distance)) {
        keep(distance, 0.0F);
    }
    if (closes && sphereInFrame(controls[1], length, start, end, distance)) {
        keep(distance, 1.0F);
    }
    const glm::vec3 axis(slope);
    const float squared = glm::dot(axis, axis) - (slope.w * slope.w);
    // where one end's sphere holds the other's, the spheres are the whole segment
    if (!(squared > kCurveRounding * glm::dot(axis, axis))) {
        return found;
    }
    // the position along is first + by * distance; the offset from the centre line, and the
    // radius, are linear in the distance as well
    const float first = ((from.w * slope.w) - glm::dot(glm::vec3(from), axis)) / squared;
    const float by = length * axis.z / squared;
    const glm::vec3 offset = -glm::vec3(from) - (axis * first);
    const glm::vec3 offsetBy = glm::vec3 { 0.0F, 0.0F, length } - (axis * by);
    const float reachBy = glm::dot(offsetBy, offsetBy);
    if (!(reachBy > 0.0F)) {
        return found;
    }
    // the quadratic is taken about the distance of the nearest offset, so a segment far off does
    // not lose its radius to rounding
    const float middle = -glm::dot(offset, offsetBy) / reachBy;
    const glm::vec3 nearest = offset + (offsetBy * middle);
    const float radius = from.w + (first * slope.w) + (by * slope.w * middle);
    const float radiusBy = by * slope.w;
    const Quadratic terms { .squared = reachBy - (radiusBy * radiusBy),
        .linear = 2.0F * (glm::dot(nearest, offsetBy) - (radius * radiusBy)),
        .constant = glm::dot(nearest, nearest) - (radius * radius) };
    float roots[2] {};
    if (quadraticRoots(terms, roots[0], roots[1]) == 0U) {
        return found;
    }
    for (const float root : roots) {
        const float crossing = middle + root;
        const float along = first + (by * crossing);
        // the next segment holds the point where this one ends
        if (crossing >= start && crossing < end && along >= 0.0F && along <= 1.0F && (along < 1.0F || closes) && radius + (radiusBy * root) >= 0.0F) {
            keep(crossing, along);
            break;
        }
    }
    return found;
}

/**
 * Whether a ray, in its own frame, meets a flat linear segment within [start, end), and where. The
 * square of the distance from the line less the square of the radius is a quadratic along it.
 * Where it curves upward its least is at its turn, held to the segment, and otherwise at an end.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - whether it closes, then the interval
MOYA_KERNEL inline bool intersectFlatLinear(const glm::vec4 * controls, bool closes, float length, float start, float end, ShapeHit & hit) {
    const glm::vec4 & from = controls[0];
    const glm::vec4 slope = controls[1] - from;
    const float squared = (slope.x * slope.x) + (slope.y * slope.y) - (slope.w * slope.w);
    const float linear = (from.x * slope.x) + (from.y * slope.y) - (from.w * slope.w);
    float along = 0.0F;
    if (squared > 0.0F) {
        along = std::fmin(1.0F, std::fmax(0.0F, -linear / squared));
    } else {
        // the quadratic at 1 less at 0
        along = squared + (2.0F * linear) < 0.0F ? 1.0F : 0.0F;
    }
    // the next segment holds the point where this one ends
    if (along >= 1.0F && !closes) {
        return false;
    }
    return flatAt(from + (slope * along), slope, along, length, start, end, hit);
}

/**
 * Whether a ray, in its own frame, meets a piece of a cubic segment within [start, end), and where
 * along the segment. The piece covers a range of positions along it. A round piece's hit is
 * refined on the swept surface, and a flat piece's at the position the ray passes deepest inside.
 * The next segment holds the point where this one ends, unless this one closes its curve.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the piece's range, then the interval
MOYA_KERNEL inline bool meetsPiece(const glm::vec4 * controls, const glm::vec4 * piece, float low, float width, bool round, bool closes, float length,
    float start, float end, float & along, float & distance) {
    guessNearest(piece, low, width, length, along, distance);
    if (!round) {
        along = refineFlat(controls, along);
        ShapeHit flat;
        if ((along >= 1.0F && !closes) || !flatAt(bezierAt(controls, along), bezierSlope(controls, along), along, length, start, end, flat)) {
            return false;
        }
        distance = flat.distance;
        return true;
    }
    // the piece's chord, as a round linear segment, gives a nearer guess at where the ray enters,
    // most of all for a ray that runs along the curve
    const glm::vec4 chord[2] = { piece[0], piece[3] };
    ShapeHit entry;
    if (intersectRoundLinear(chord, true, length, start, std::numeric_limits<float>::infinity(), entry)) {
        along = low + (entry.uv.y * width);
        distance = entry.distance;
    }
    return refineRound(controls, length, along, distance) && (along < 1.0F || closes) && distance >= start && distance < end;
}

/**
 * Whether a ray, in its own frame, meets a cubic segment within [start, end), and where. The
 * segment is halved kCurveDepth times, passing over each piece the ray cannot meet, and a hit on
 * each piece left is refined from a guess. A round segment's ends are the spheres there.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the profile, then the interval
MOYA_KERNEL inline bool intersectCubic(const glm::vec4 * controls, bool round, bool closes, float length, float start, float end, ShapeHit & hit) {
    bool found = false;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, then the position
    const auto keep = [&](float distance, float along) {
        end = distance;
        hit.distance = distance;
        hit.uv = glm::vec2 { acrossOf(bezierAt(controls, along), bezierSlope(controls, along)), along };
        found = true;
    };
    float distance = 0.0F;
    if (round && sphereInFrame(controls[0], length, start, end, distance)) {
        keep(distance, 0.0F);
    }
    if (round && closes && sphereInFrame(controls[3], length, start, end, distance)) {
        keep(distance, 1.0F);
    }
    // the pieces still to search, depth first, the first half of each before its second
    glm::vec4 pieces[kCurveDepth + 1][4];
    float lows[kCurveDepth + 1] {};
    std::uint32_t levels[kCurveDepth + 1] {};
    for (std::uint32_t at = 0; at < 4; ++at) {
        pieces[0][at] = controls[at];
    }
    std::uint32_t count = 1;
    while (count > 0) {
        --count;
        const float low = lows[count];
        const std::uint32_t level = levels[count];
        if (!mayMeet(pieces[count], length, start, end)) {
            continue;
        }
        const float width = 1.0F / static_cast<float>(1U << level);
        if (level == kCurveDepth) {
            float along = 0.0F;
            if (meetsPiece(controls, pieces[count], low, width, round, closes, length, start, end, along, distance)) {
                keep(distance, along);
            }
            continue;
        }
        glm::vec4 first[4];
        glm::vec4 second[4];
        halve(pieces[count], first, second);
        for (std::uint32_t at = 0; at < 4; ++at) {
            pieces[count][at] = second[at];
            pieces[count + 1][at] = first[at];
        }
        lows[count] = low + (width * 0.5F);
        lows[count + 1] = low;
        levels[count] = level + 1;
        levels[count + 1] = level + 1;
        count += 2;
    }
    return found;
}

/**
 * Whether a ray meets a segment of curves of a form within [start, end), and where. The segment's
 * controlsOf(form) control points are in its own space, as the ray is. It holds its end only when
 * it closes its curve. A ray whose direction has no length meets nothing.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the segment, then the interval
MOYA_KERNEL inline bool intersectCurve(Form form, const glm::vec4 * controls, bool closes, const Ray & ray, float start, float end, ShapeHit & hit) {
    const RayFrame frame = RayFrame::of(ray);
    if (!(frame.length > 0.0F)) {
        return false;
    }
    glm::vec4 own[4];
    const std::uint32_t count = controlsOf(form);
    for (std::uint32_t at = 0; at < count; ++at) {
        own[at] = frame.into(controls[at]);
    }
    switch (form) {
        case Form::RoundLinearCurves:
            return intersectRoundLinear(own, closes, frame.length, start, end, hit);
        case Form::FlatLinearCurves:
            return intersectFlatLinear(own, closes, frame.length, start, end, hit);
        case Form::RoundCubicCurves:
            return intersectCubic(own, true, closes, frame.length, start, end, hit);
        case Form::FlatCubicCurves:
            return intersectCubic(own, false, closes, frame.length, start, end, hit);
        case Form::Triangles:
        case Form::RoundPoints:
        case Form::FlatPoints:
        case Form::Quadric:
            break;
    }
    return false;
}

/**
 * A curve's geometric normal at a point a hit met, in the segment's own space and not of unit
 * length. A round curve's points away from its centre line at the hit's position along it. A flat
 * curve's, and any other form's, faces back along the ray.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the position along
MOYA_KERNEL inline glm::vec3 curveNormal(Form form, const glm::vec4 * controls, const glm::vec3 & point, float along, const glm::vec3 & direction) {
    switch (form) {
        case Form::RoundLinearCurves:
            return point - glm::vec3(controls[0] + ((controls[1] - controls[0]) * along));
        case Form::RoundCubicCurves:
            return point - glm::vec3(bezierAt(controls, along));
        case Form::FlatLinearCurves:
        case Form::FlatCubicCurves:
        case Form::Triangles:
        case Form::RoundPoints:
        case Form::FlatPoints:
        case Form::Quadric:
            break;
    }
    return -direction;
}

};  // namespace v3d::moya::kernel::geometry::primitive
