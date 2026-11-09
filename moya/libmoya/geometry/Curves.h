/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Segments.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

/**
 * How many varying values a set of curves takes, as RI counts them: one at each end of each
 * segment, shared between neighbours, so a periodic curve takes one per segment. A linear curve's
 * are its vertices.
 *
 * A curve's segments follow RI. A linear curve has one between each pair of vertices, and a
 * periodic one a last that closes it. A cubic curve has one at every step of its basis that four
 * vertices from there reach, and a periodic one wraps back to its first vertices. The bezier
 * basis steps by 3, b-spline and catmull-rom by 1, hermite by 2, and power by 4.
 *
 * Throws std::invalid_argument for a curve of fewer than two vertices, or whose vertices do not
 * make whole segments.
 **/
std::size_t varyingCount(const std::vector<std::int32_t> & vertices, bool cubic, const std::string & basis, bool periodic);

/**
 * A set of curves cut into segments, a cubic curve's converted exactly to Bézier segments from
 * its basis.
 *
 * Each width is the curve's diameter there, given one per varying value or one per vertex, and a
 * radius is half of it. A varying width runs linearly along its segment, and a vertex's is
 * converted by the basis as the points are. Where the two counts agree, the widths are varying,
 * as RI declares them. A radius that the basis takes below zero is held at zero. The last
 * segment of a curve that is not periodic closes it.
 *
 * Throws std::invalid_argument for the reasons varyingCount() does, for counts of vertices that
 * do not add up to the points, and for widths of neither count.
 **/
Segments cut(const std::vector<glm::vec3> & points, const std::vector<float> & widths, const std::vector<std::int32_t> & vertices, bool cubic,
    const std::string & basis, bool periodic);

};  // namespace v3d::moya::geometry
