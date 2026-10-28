/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bases.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Requests.h"

#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

namespace {

constexpr float SIXTH = 1.0F / 6.0F;
constexpr float TWO_THIRDS = 4.0F / 6.0F;

/**
 * A cubic's degree. Its Bezier handles lie a third of the way along, and its derivative is
 * this many times the steps between its points.
 **/
constexpr float DEGREE = 3.0F;
constexpr float THIRD = 1.0F / DEGREE;

/**
 * How many segments one curve of a Curves request has.
 **/
std::size_t segmentCount(std::size_t points, bool cubic, bool periodic, unsigned int step) {
    if (!cubic) {
        if (points < 2) {
            return 0;
        }
        return periodic ? points : points - 1;
    }
    if (step == 0 || points < 4) {
        return 0;
    }
    return periodic ? points / step : ((points - 4) / step) + 1;
}

/**
 * RI's standard bases, as its specification gives them.
 **/
constexpr auto STANDARD = std::to_array<std::pair<const char*, Basis::Matrix>>({
    { "bezier", Basis::BEZIER },
    { "b-spline", { -SIXTH, 0.5F, -0.5F, SIXTH, 0.5F, -1, 0.5F, 0, -0.5F, 0, 0.5F, 0, SIXTH, TWO_THIRDS, SIXTH, 0 } },
    { "catmull-rom", { -0.5F, 1.5F, -1.5F, 0.5F, 1, -2.5F, 2, -0.5F, -0.5F, 0, 0.5F, 0, 0, 1, 0, 0 } },
    { "hermite", { 2, 1, -2, 1, -3, -2, 3, -1, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "power", { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } },
});

};  // namespace

bool namedBasis(const std::string & name, Basis::Matrix * matrix) {
    const auto found = std::ranges::find_if(STANDARD, [&name](const auto& known) { return name == known.first; });
    if (found == STANDARD.end()) {
        return false;
    }
    *matrix = found->second;
    return true;
}

std::string basisName(const Basis::Matrix & matrix) {
    const auto found = std::ranges::find_if(STANDARD, [&matrix](const auto& known) { return matrix == known.second; });
    return found == STANDARD.end() ? std::string() : std::string(found->first);
}

Segment toBezier(const Basis::Matrix & basis, const Segment & points) {
    // the basis times the points gives the cubic's coefficients, of t^3, t^2, t and 1 in that
    // order, and those give the Bezier points that make the same cubic
    std::array<glm::vec3, 4> coefficient{};
    for (std::size_t row = 0; row < 4; row++) {
        coefficient[row] = (basis[row * 4] * points[0]) + (basis[(row * 4) + 1] * points[1]) +
            (basis[(row * 4) + 2] * points[2]) + (basis[(row * 4) + 3] * points[3]);
    }
    Segment bezier{};
    bezier[0] = coefficient[3];
    bezier[1] = bezier[0] + (coefficient[2] * THIRD);
    bezier[2] = (coefficient[1] * THIRD) + (2.0F * bezier[1]) - bezier[0];
    bezier[3] = coefficient[0] + bezier[0] + (DEGREE * (bezier[2] - bezier[1]));
    return bezier;
}

std::vector<Segment> segments(const std::vector<glm::vec3> & points, bool cubic, bool periodic,
    const Basis::Matrix & basis, unsigned int step) {
    std::vector<Segment> made;
    const std::size_t count = points.size();
    const auto at = [&points, count](std::size_t index) { return points[index % count]; };
    const std::size_t pieces = segmentCount(count, cubic, periodic, step);
    if (!cubic) {
        // a straight segment is the Bezier segment with its handles a third of the way along
        for (std::size_t line = 0; line < pieces; line++) {
            const glm::vec3 from = at(line);
            const glm::vec3 to = at(line + 1);
            made.push_back({ from, from + ((to - from) * THIRD), to - ((to - from) * THIRD), to });
        }
        return made;
    }
    for (std::size_t piece = 0; piece < pieces; piece++) {
        const std::size_t first = piece * step;
        made.push_back(toBezier(basis, { at(first), at(first + 1), at(first + 2), at(first + 3) }));
    }
    return made;
}

std::size_t varyingCount(std::size_t points, bool cubic, bool periodic, unsigned int step) {
    const std::size_t pieces = segmentCount(points, cubic, periodic, step);
    if (pieces == 0) {
        return 0;
    }
    return periodic ? pieces : pieces + 1;
}

};  // namespace v3d::render::offline::rib
