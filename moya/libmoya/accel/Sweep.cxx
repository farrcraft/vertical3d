/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Sweep.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/internal/Wave.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::accel {

namespace {

/** The float at or below a double. **/
float down(double value) {
    const auto out = static_cast<float>(value);
    return static_cast<double>(out) > value ? std::nextafter(out, -std::numeric_limits<float>::infinity()) : out;
}

/** The float at or above a double. **/
float up(double value) {
    const auto out = static_cast<float>(value);
    return static_cast<double>(out) < value ? std::nextafter(out, std::numeric_limits<float>::infinity()) : out;
}

/** The eight corners of a box. **/
std::array<glm::dvec3, 8> corners(const Box & box) {
    std::array<glm::dvec3, 8> out;
    for (std::size_t corner = 0; corner < out.size(); ++corner) {
        out[corner] = glm::dvec3 { (corner & 1U) != 0 ? box.upper.x : box.lower.x, (corner & 2U) != 0 ? box.upper.y : box.lower.y,
            (corner & 4U) != 0 ? box.upper.z : box.lower.z };
    }
    return out;
}

/** The segment of keys a time lies in: the key at or before it. Beyond the last, the last. **/
std::size_t segmentOf(const std::vector<kernel::geometry::Key> & keys, double time) {
    std::size_t out = 0;
    while (out + 1 < keys.size() && time >= static_cast<double>(keys[out + 1].time)) {
        ++out;
    }
    return out;
}

/** The coordinate along an axis of a point carried by keys, at a time. Outside them it holds. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the axis, then the time
double carriedAt(const std::vector<kernel::geometry::Key> & keys, const glm::dvec3 & point, int axis, double time) {
    if (time <= static_cast<double>(keys.front().time)) {
        return internal::Wave::carried(keys.front(), keys.front(), point, axis).at(0.0);
    }
    if (time >= static_cast<double>(keys.back().time)) {
        return internal::Wave::carried(keys.back(), keys.back(), point, axis).at(0.0);
    }
    const std::size_t at = segmentOf(keys, time);
    const double from = keys[at].time;
    const double span = static_cast<double>(keys[at + 1].time) - from;
    return internal::Wave::carried(keys[at], keys[at + 1], point, axis).at((time - from) / span);
}

/**
 * The furthest a coordinate of a point carried by keys strays from a line over a time range: over
 * it, or under it. line holds the range's begin and end, and the line's value at each. Negative
 * when it never strays.
 **/
double strays(const std::vector<kernel::geometry::Key> & keys, const glm::dvec3 & point, int axis, const std::array<double, 4> & line, bool over) {
    const double begin = line[0];
    const double end = line[1];
    const double first = line[2];
    const double slope = (line[3] - first) / (end - begin);
    std::vector<double> cuts { begin };
    for (const kernel::geometry::Key & key : keys) {
        if (static_cast<double>(key.time) > begin && static_cast<double>(key.time) < end) {
            cuts.push_back(key.time);
        }
    }
    cuts.push_back(end);
    double out = -std::numeric_limits<double>::infinity();
    for (std::size_t piece = 0; piece + 1 < cuts.size(); ++piece) {
        const double from = cuts[piece];
        const double to = cuts[piece + 1];
        const double middle = (from + to) * 0.5;
        if (middle <= static_cast<double>(keys.front().time) || middle >= static_cast<double>(keys.back().time)) {
            // the point holds still here, so it strays furthest at an end of the piece
            for (const double time : { from, to }) {
                const double gap = carriedAt(keys, point, axis, time) - (first + (slope * (time - begin)));
                out = std::max(out, over ? gap : -gap);
            }
            continue;
        }
        const std::size_t at = segmentOf(keys, middle);
        const double keyTime = keys[at].time;
        const double span = static_cast<double>(keys[at + 1].time) - keyTime;
        const internal::Wave wave = internal::Wave::carried(keys[at], keys[at + 1], point, axis);
        // the line as the segment reads it, from its first key to its next
        const double value = first + (slope * (keyTime - begin));
        const double rise = slope * span;
        const internal::Wave gap = over ? wave.overLine(value, rise) : wave.underLine(value, rise);
        out = std::max(out, gap.greatest((from - keyTime) / span, (to - keyTime) / span));
    }
    return out;
}

/** A coordinate of a mesh's point at a time, interpolated in double. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, the axis, then the time
double coordinateOf(std::uint32_t index, int axis, double time, const kernel::geometry::Sampled & held, const std::vector<glm::vec3> & points,
    const std::vector<float> & times) {
    const auto coordinate = [&](std::uint32_t sample) {
        return static_cast<double>(points[held.first + (sample * held.count) + index][axis]);
    };
    if (held.samples < 2 || time <= static_cast<double>(times[held.times])) {
        return coordinate(0);
    }
    for (std::uint32_t sample = 0; sample + 1 < held.samples; ++sample) {
        const double from = times[held.times + sample];
        const double to = times[held.times + sample + 1];
        if (time < to) {
            const double along = (time - from) / (to - from);
            return coordinate(sample) + ((coordinate(sample + 1) - coordinate(sample)) * along);
        }
    }
    return coordinate(held.samples - 1);
}

/**
 * Bounds one axis of a triangle that moves between samples over a time range, as triangleSweep()
 * describes.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the triangle, the axis, then the range
void sweepTriangle(std::uint32_t primitive, int axis, float begin, float end, const kernel::geometry::Sampled & held,
    const std::vector<glm::vec3> & points, const std::vector<float> & times, LinearBox & out) {
    const auto at = [&](std::uint32_t corner, double time) {
        return coordinateOf((primitive * 3) + corner, axis, time, held, points, times);
    };
    double lower[2] = { std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() };
    double upper[2] = { -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity() };
    for (std::uint32_t corner = 0; corner < 3; ++corner) {
        lower[0] = std::min(lower[0], at(corner, begin));
        lower[1] = std::min(lower[1], at(corner, end));
        upper[0] = std::max(upper[0], at(corner, begin));
        upper[1] = std::max(upper[1], at(corner, end));
    }
    // between samples each point moves along a line, so the lines need only hold at the samples
    double under = 0.0;
    double over = 0.0;
    for (std::uint32_t sample = 0; sample < held.samples; ++sample) {
        const double time = times[held.times + sample];
        if (time <= static_cast<double>(begin) || time >= static_cast<double>(end)) {
            continue;
        }
        const double along = (time - begin) / (static_cast<double>(end) - begin);
        for (std::uint32_t corner = 0; corner < 3; ++corner) {
            const double value = at(corner, time);
            under = std::max(under, (lower[0] + ((lower[1] - lower[0]) * along)) - value);
            over = std::max(over, value - (upper[0] + ((upper[1] - upper[0]) * along)));
        }
    }
    out.start.lower[axis] = down(lower[0] - under);
    out.end.lower[axis] = down(lower[1] - under);
    out.start.upper[axis] = up(upper[0] + over);
    out.end.upper[axis] = up(upper[1] + over);
}

};  // namespace

Box LinearBox::at(float along) const {
    if (start.empty() || end.empty()) {
        return start.empty() ? end : start;
    }
    return Box { .lower = start.lower + ((end.lower - start.lower) * along), .upper = start.upper + ((end.upper - start.upper) * along) };
}

float LinearBox::area() const {
    return (start.area() + (kMiddle * at(0.5F).area()) + end.area()) / kWeights;
}

Box LinearBox::swept() const {
    Box out = start;
    out.grow(end);
    return out;
}

Sweep triangleSweep(const kernel::geometry::Sampled & held, const std::vector<glm::vec3> & points, const std::vector<float> & times) {
    return [held, &points, &times](std::uint32_t primitive, float begin, float end) {
        LinearBox out;
        for (int axis = 0; axis < 3; ++axis) {
            sweepTriangle(primitive, axis, begin, end, held, points, times, out);
        }
        return out;
    };
}

LinearBox sweepBox(const Box & box, const std::vector<kernel::geometry::Key> & keys, float begin, float end) {
    if (box.empty() || keys.empty()) {
        return {};
    }
    const std::array<glm::dvec3, 8> points = corners(box);
    LinearBox out;
    for (int axis = 0; axis < 3; ++axis) {
        double lower[2] = { std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() };
        double upper[2] = { -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity() };
        for (const glm::dvec3 & point : points) {
            for (int side = 0; side < 2; ++side) {
                const double value = carriedAt(keys, point, axis, side == 0 ? begin : end);
                lower[side] = std::min(lower[side], value);
                upper[side] = std::max(upper[side], value);
            }
        }
        double under = 0.0;
        double over = 0.0;
        if (end > begin) {
            for (const glm::dvec3 & point : points) {
                under = std::max(under, strays(keys, point, axis, { begin, end, lower[0], lower[1] }, false));
                over = std::max(over, strays(keys, point, axis, { begin, end, upper[0], upper[1] }, true));
            }
        }
        out.start.lower[axis] = down(lower[0] - under);
        out.end.lower[axis] = down(lower[1] - under);
        out.start.upper[axis] = up(upper[0] + over);
        out.end.upper[axis] = up(upper[1] + over);
    }
    return out;
}

Sweep pointSweep(const kernel::geometry::Sampled & centres, const kernel::geometry::Sampled & widths, const std::vector<glm::vec3> & points,
    const std::vector<float> & widthValues, const std::vector<float> & times) {
    return [centres, widths, &points, &widthValues, &times](std::uint32_t primitive, float begin, float end) {
        // each extent is a centre less or more half a width, which moves linearly between the
        // samples of both, so it need only hold at those
        std::vector<double> breaks;
        for (const kernel::geometry::Sampled * held : { &centres, &widths }) {
            for (std::uint32_t sample = 0; sample < held->samples && held->samples > 1; ++sample) {
                const double time = times[held->times + sample];
                if (time > static_cast<double>(begin) && time < static_cast<double>(end)) {
                    breaks.push_back(time);
                }
            }
        }
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the time
        const auto extent = [&](int axis, double side, double time) {
            const float at = static_cast<float>(time);
            const glm::vec3 centre = kernel::geometry::sampleAt(centres, points.data(), times.data(), primitive, at);
            const float width = kernel::geometry::sampleAt(widths, widthValues.data(), times.data(), primitive, at);
            return static_cast<double>(centre[axis]) + (side * 0.5 * static_cast<double>(width));
        };
        LinearBox out;
        for (int axis = 0; axis < 3; ++axis) {
            const double lower[2] = { extent(axis, -1.0, begin), extent(axis, -1.0, end) };
            const double upper[2] = { extent(axis, 1.0, begin), extent(axis, 1.0, end) };
            double under = 0.0;
            double over = 0.0;
            for (const double time : breaks) {
                const double along = (time - begin) / (static_cast<double>(end) - begin);
                under = std::max(under, (lower[0] + ((lower[1] - lower[0]) * along)) - extent(axis, -1.0, time));
                over = std::max(over, extent(axis, 1.0, time) - (upper[0] + ((upper[1] - upper[0]) * along)));
            }
            out.start.lower[axis] = down(lower[0] - under);
            out.end.lower[axis] = down(lower[1] - under);
            out.start.upper[axis] = up(upper[0] + over);
            out.end.upper[axis] = up(upper[1] + over);
        }
        return out;
    };
}

Sweep curveSweep(const kernel::geometry::Sampled & held, std::uint32_t perSegment, const std::vector<glm::vec4> & controls,
    const std::vector<float> & times) {
    return [held, perSegment, &controls, &times](std::uint32_t primitive, float begin, float end) {
        // each extent is the least or greatest of the control points' centres less or more their
        // radii. Each of those moves linearly between samples, so the least is concave between
        // them and the greatest convex, and the bounds need only hold at the samples
        std::vector<double> breaks;
        for (std::uint32_t sample = 0; sample < held.samples && held.samples > 1; ++sample) {
            const double time = times[held.times + sample];
            if (time > static_cast<double>(begin) && time < static_cast<double>(end)) {
                breaks.push_back(time);
            }
        }
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the time
        const auto extent = [&](int axis, double side, double time) {
            const float at = static_cast<float>(time);
            double out = side * -std::numeric_limits<double>::infinity();
            for (std::uint32_t control = 0; control < perSegment; ++control) {
                const glm::vec4 point = kernel::geometry::sampleAt(held, controls.data(), times.data(), (primitive * perSegment) + control, at);
                const double reach = static_cast<double>(point[axis]) + (side * static_cast<double>(point.w));
                out = side > 0.0 ? std::max(out, reach) : std::min(out, reach);
            }
            return out;
        };
        LinearBox out;
        for (int axis = 0; axis < 3; ++axis) {
            const double lower[2] = { extent(axis, -1.0, begin), extent(axis, -1.0, end) };
            const double upper[2] = { extent(axis, 1.0, begin), extent(axis, 1.0, end) };
            double under = 0.0;
            double over = 0.0;
            for (const double time : breaks) {
                const double along = (time - begin) / (static_cast<double>(end) - begin);
                under = std::max(under, (lower[0] + ((lower[1] - lower[0]) * along)) - extent(axis, -1.0, time));
                over = std::max(over, extent(axis, 1.0, time) - (upper[0] + ((upper[1] - upper[0]) * along)));
            }
            out.start.lower[axis] = down(lower[0] - under);
            out.end.lower[axis] = down(lower[1] - under);
            out.start.upper[axis] = up(upper[0] + over);
            out.end.upper[axis] = up(upper[1] + over);
        }
        return out;
    };
}

Box quadricBounds(const kernel::geometry::primitive::Quadric & least, const kernel::geometry::primitive::Quadric & most) {
    const auto largest = [](float first, float second) {
        return std::max(std::fabs(first), std::fabs(second));
    };
    float reach = largest(least.radius, most.radius);
    float lower = 0.0F;
    float upper = 0.0F;
    switch (least.shape) {
        case kernel::geometry::primitive::Shape::Sphere:
            lower = std::max(-reach, std::min(least.zmin, least.zmax));
            upper = std::min(reach, std::max(most.zmin, most.zmax));
            break;
        case kernel::geometry::primitive::Shape::Cylinder:
        case kernel::geometry::primitive::Shape::Paraboloid:
            lower = std::min(least.zmin, least.zmax);
            upper = std::max(most.zmin, most.zmax);
            break;
        case kernel::geometry::primitive::Shape::Cone:
            lower = std::min(0.0F, least.height);
            upper = std::max(0.0F, most.height);
            break;
        case kernel::geometry::primitive::Shape::Disk:
            lower = least.height;
            upper = most.height;
            break;
        case kernel::geometry::primitive::Shape::Hyperboloid: {
            // the line's farthest reach from z is at one of its ends
            const float x = std::max(largest(least.point1.x, most.point1.x), largest(least.point2.x, most.point2.x));
            const float y = std::max(largest(least.point1.y, most.point1.y), largest(least.point2.y, most.point2.y));
            reach = std::sqrt((x * x) + (y * y));
            lower = std::min(least.point1.z, least.point2.z);
            upper = std::max(most.point1.z, most.point2.z);
            break;
        }
        case kernel::geometry::primitive::Shape::Torus: {
            const float tube = largest(least.minorRadius, most.minorRadius);
            reach += tube;
            lower = -tube;
            upper = tube;
            break;
        }
    }
    return Box { .lower = { -reach, -reach, lower }, .upper = { reach, reach, upper } };
}

LinearBox quadricSweep(const kernel::geometry::Sampled & held, const std::vector<kernel::geometry::primitive::Quadric> & quadrics,
    const std::vector<float> & times, float begin, float end) {
    std::vector<kernel::geometry::primitive::Quadric> standing { kernel::geometry::primitive::sampledQuadric(held, quadrics.data(), times.data(), begin),
        kernel::geometry::primitive::sampledQuadric(held, quadrics.data(), times.data(), end) };
    for (std::uint32_t sample = 0; sample < held.samples && held.samples > 1; ++sample) {
        const float time = times[held.times + sample];
        if (time > begin && time < end) {
            standing.push_back(quadrics[held.first + sample]);
        }
    }
    kernel::geometry::primitive::Quadric least = standing.front();
    kernel::geometry::primitive::Quadric most = standing.front();
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the least, then the greatest
    const auto bound = [](float & low, float & high, float value) {
        low = std::min(low, value);
        high = std::max(high, value);
    };
    for (const kernel::geometry::primitive::Quadric & quadric : standing) {
        bound(least.radius, most.radius, quadric.radius);
        bound(least.minorRadius, most.minorRadius, quadric.minorRadius);
        bound(least.zmin, most.zmin, quadric.zmin);
        bound(least.zmax, most.zmax, quadric.zmax);
        bound(least.height, most.height, quadric.height);
        for (int axis = 0; axis < 3; ++axis) {
            bound(least.point1[axis], most.point1[axis], quadric.point1[axis]);
            bound(least.point2[axis], most.point2[axis], quadric.point2[axis]);
        }
    }
    return LinearBox::still(quadricBounds(least, most));
}

Box moveBox(const Box & box, const glm::mat4 & transform) {
    Box out;
    if (box.empty()) {
        return out;
    }
    for (const glm::dvec3 & corner : corners(box)) {
        out.grow(glm::vec3(transform * glm::vec4(glm::vec3(corner), 1.0F)));
    }
    return out;
}

LinearBox moveBox(const LinearBox & box, const glm::mat4 & transform) {
    return LinearBox { .start = moveBox(box.start, transform), .end = moveBox(box.end, transform) };
}

};  // namespace v3d::moya::accel
