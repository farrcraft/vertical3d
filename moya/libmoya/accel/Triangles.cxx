/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Triangles.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::accel {

namespace {

/** A triangle clipped by six planes has at most nine corners. **/
constexpr std::size_t kMost = 9;

using Polygon = std::array<glm::vec3, kMost>;

/**
 * Clips a convex polygon of count corners to one side of the plane where the axis equals plane:
 * above it for a side of 1, below it for -1. Returns the corners left.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the plane, then which side of it is kept
std::size_t clipToPlane(Polygon & polygon, std::size_t count, int axis, float plane, float side) {
    Polygon kept {};
    std::size_t keptCount = 0;
    for (std::size_t i = 0; i < count && keptCount < kMost; ++i) {
        const glm::vec3 & here = polygon[i];
        const glm::vec3 & next = polygon[(i + 1) % count];
        const float dHere = side * (here[axis] - plane);
        const float dNext = side * (next[axis] - plane);
        if (dHere >= 0.0F) {
            kept[keptCount++] = here;
        }
        if (((dHere < 0.0F) != (dNext < 0.0F)) && keptCount < kMost) {
            glm::vec3 crossing = here + ((next - here) * (dHere / (dHere - dNext)));
            // the crossing lies on the plane, whatever its rounding gives
            crossing[axis] = plane;
            kept[keptCount++] = crossing;
        }
    }
    polygon = kept;
    return keptCount;
}

};  // namespace

std::vector<Box> triangleBounds(const std::vector<glm::vec3> & points) {
    std::vector<Box> out(points.size() / 3);
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i].grow(points[i * 3]);
        out[i].grow(points[(i * 3) + 1]);
        out[i].grow(points[(i * 3) + 2]);
    }
    return out;
}

Box clipTriangle(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2, const Box & within) {
    Polygon polygon { p0, p1, p2 };
    std::size_t count = 3;
    for (int axis = 0; axis < 3 && count > 0; ++axis) {
        count = clipToPlane(polygon, count, axis, within.lower[axis], 1.0F);
        if (count > 0) {
            count = clipToPlane(polygon, count, axis, within.upper[axis], -1.0F);
        }
    }
    Box out;
    for (std::size_t i = 0; i < count; ++i) {
        out.grow(polygon[i]);
    }
    return out.within(within);
}

Clip triangleClip(const std::vector<glm::vec3> & points) {
    return [&points](std::uint32_t primitive, const Box & within) {
        const std::size_t first = static_cast<std::size_t>(primitive) * 3;
        return clipTriangle(points[first], points[first + 1], points[first + 2], within);
    };
}

};  // namespace v3d::moya::accel
