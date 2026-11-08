/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Sweep.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/internal/Wave.h>
#include <moya/libmoya/kernel/geometry/Motion.h>

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

/** Simpson's rule over three points: the weight of the middle one, and the sum of the weights. **/
constexpr float kMiddle = 4.0F;
constexpr float kWeights = 6.0F;

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
double pointAt(std::uint32_t index, int axis, double time, const kernel::geometry::PointSamples & held, const std::vector<glm::vec3> & points,
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
void sweepTriangle(std::uint32_t primitive, int axis, float begin, float end, const kernel::geometry::PointSamples & held,
    const std::vector<glm::vec3> & points, const std::vector<float> & times, LinearBox & out) {
    const auto at = [&](std::uint32_t corner, double time) {
        return pointAt((primitive * 3) + corner, axis, time, held, points, times);
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

Sweep triangleSweep(const kernel::geometry::PointSamples & held, const std::vector<glm::vec3> & points, const std::vector<float> & times) {
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
