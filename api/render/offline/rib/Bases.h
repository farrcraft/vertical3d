/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "Requests.h"

#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * The basis matrix RI defines for a name: "bezier", "b-spline", "catmull-rom", "hermite" or
 * "power".
 *
 * @return whether the name is one of them
 **/
bool namedBasis(const std::string & name, Basis::Matrix * matrix);

/**
 * @return the name RI gives a basis matrix, or empty for a matrix that is not one of its own
 **/
std::string basisName(const Basis::Matrix & matrix);

/**
 * Four points in order, such as one segment's control points.
 **/
using Segment = std::array<glm::vec3, 4>;

/**
 * The control points of the Bezier segment that is the same curve as four control points
 * under a basis matrix. A curve under any basis is drawn as such segments.
 **/
Segment toBezier(const Basis::Matrix & basis, const Segment & points);

/**
 * The segments of one curve of a Curves request, each as the Bezier segment it is, from the
 * curve's own run of control points.
 *
 * A linear curve has a segment between each pair of neighbouring points. A cubic one has a
 * segment for each step of the basis that leaves four points ahead, and a periodic curve wraps
 * round to its first points for the last ones. A curve with too few points for one segment has
 * none.
 *
 * @param cubic whether the curve is cubic, in which case basis and step are the v direction's
 **/
std::vector<Segment> segments(const std::vector<glm::vec3> & points, bool cubic, bool periodic,
    const Basis::Matrix & basis, unsigned int step);

/**
 * How many varying values one curve of a Curves request has: one at each end of each segment,
 * and on a periodic curve the last segment's end is the first one's start.
 **/
std::size_t varyingCount(std::size_t points, bool cubic, bool periodic, unsigned int step);

};  // namespace v3d::render::offline::rib
