/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Curves.h>

#include <moya/libmoya/geometry/Segments.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry {

namespace {

/** A 4 by 4 matrix, rows first. **/
using Matrix = std::array<double, 16>;

/** RI's step from one cubic segment's first vertex to the next's, for a basis. **/
std::uint32_t stepOf(const std::string & basis) {
    if (basis == "b-spline" || basis == "catmull-rom") {
        return 1;
    }
    if (basis == "hermite") {
        return 2;
    }
    if (basis == "power") {
        return 4;
    }
    return 3;
}

/**
 * RI's matrix for a basis, which takes a segment's four vertices to the coefficients of u^3, u^2,
 * u and 1 along it.
 **/
Matrix matrixOf(const std::string & basis) {
    constexpr double kSixth = 1.0 / 6.0;
    if (basis == "b-spline") {
        return { -kSixth, 0.5, -0.5, kSixth, 0.5, -1.0, 0.5, 0.0, -0.5, 0.0, 0.5, 0.0, kSixth, 4.0 * kSixth, kSixth, 0.0 };  // NOLINT(readability-magic-numbers)
    }
    if (basis == "catmull-rom") {
        return { -0.5, 1.5, -1.5, 0.5, 1.0, -2.5, 2.0, -0.5, -0.5, 0.0, 0.5, 0.0, 0.0, 1.0, 0.0, 0.0 };  // NOLINT(readability-magic-numbers)
    }
    if (basis == "hermite") {
        return { 2.0, 1.0, -2.0, 1.0, -3.0, -2.0, 3.0, -1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 };  // NOLINT(readability-magic-numbers)
    }
    if (basis == "power") {
        return { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
    }
    return { -1.0, 3.0, -3.0, 1.0, 3.0, -6.0, 3.0, 0.0, -3.0, 3.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 };  // NOLINT(readability-magic-numbers)
}

/**
 * The matrix that takes a segment's four vertices in a basis to its Bézier control points: RI's
 * matrix for the basis, then the inverse of the Bézier basis's.
 **/
Matrix toBezier(const std::string & basis) {
    constexpr double kThird = 1.0 / 3.0;
    // takes the coefficients of u^3, u^2, u and 1 to Bézier control points
    constexpr Matrix kFromPower { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, kThird, 1.0, 0.0, kThird, 2.0 * kThird, 1.0, 1.0, 1.0, 1.0, 1.0 };
    const Matrix basisMatrix = matrixOf(basis);
    Matrix out {};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            for (std::size_t inner = 0; inner < 4; ++inner) {
                out[(row * 4) + column] += kFromPower[(row * 4) + inner] * basisMatrix[(inner * 4) + column];
            }
        }
    }
    return out;
}

/** How many segments a curve of a count of vertices has. Throws when they make none whole. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count, then the step
std::size_t segmentsOf(std::int32_t count, bool cubic, std::uint32_t step, bool periodic) {
    const auto whole = static_cast<std::size_t>(std::max(count, 0));
    if (count < 2) {
        throw std::invalid_argument("a curve of " + std::to_string(count) + " vertices has no segment");
    }
    if (!cubic) {
        return periodic ? whole : whole - 1;
    }
    if (periodic ? whole % step != 0 : (whole < 4 || (whole - 4) % step != 0)) {
        throw std::invalid_argument("a cubic curve of " + std::to_string(count) + " vertices does not make whole segments at a step of " +
            std::to_string(step));
    }
    return periodic ? whole / step : ((whole - 4) / step) + 1;
}

/** How many varying values a curve with a count of segments takes. **/
std::size_t varyingOf(std::size_t segments, bool periodic) {
    return periodic ? segments : segments + 1;
}

/**
 * Whether a set of curves' widths are one per varying value rather than one per vertex. Throws
 * when the counts of vertices do not add up to the points, or the widths are of neither count.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the counts of points, then of widths
bool perVaryingValue(std::size_t points, std::size_t widths, const std::vector<std::int32_t> & vertices, std::size_t varying) {
    std::size_t total = 0;
    for (const std::int32_t count : vertices) {
        total += static_cast<std::size_t>(count);
    }
    if (total != points) {
        throw std::invalid_argument("its counts of vertices do not add up to its points");
    }
    if (widths != varying && widths != points) {
        throw std::invalid_argument("its widths are neither one per vertex nor one per varying value");
    }
    return widths == varying;
}

/**
 * Appends a linear segment's two control points: its vertices, each with its own radius or with
 * the radius a varying width gives at its end.
 **/
void appendLinear(const std::array<glm::dvec4, 4> & given, const std::optional<std::array<double, 2>> & varying, Segments & out) {
    for (std::size_t end = 0; end < 2; ++end) {
        const double radius = varying ? (*varying)[end] : given[end].w;
        out.controls.emplace_back(glm::vec3(given[end]), static_cast<float>(std::max(0.0, radius)));
    }
}

/**
 * Appends a cubic segment's Bézier control points, which a basis's matrix takes from its four
 * vertices, each vertex's radius with it. Where a varying width gives the radii at the segment's
 * ends, the radius runs linearly between them instead.
 **/
void appendCubic(const std::array<glm::dvec4, 4> & given, const Matrix & matrix, const std::optional<std::array<double, 2>> & varying,
    Segments & out) {
    constexpr double kCubic = 3.0;
    for (std::size_t row = 0; row < 4; ++row) {
        glm::dvec4 control { 0.0 };
        for (std::size_t corner = 0; corner < 4; ++corner) {
            control += given[corner] * matrix[(row * 4) + corner];
        }
        if (varying) {
            // a radius linear along the segment has its Bézier controls at even steps
            control.w = (*varying)[0] + (((*varying)[1] - (*varying)[0]) * static_cast<double>(row) / kCubic);
        }
        out.controls.emplace_back(glm::vec3(control), static_cast<float>(std::max(0.0, control.w)));
    }
}

};  // namespace

std::size_t varyingCount(const std::vector<std::int32_t> & vertices, bool cubic, const std::string & basis, bool periodic) {
    const std::uint32_t step = stepOf(basis);
    std::size_t out = 0;
    for (const std::int32_t count : vertices) {
        const std::size_t segments = segmentsOf(count, cubic, step, periodic);
        out += cubic ? varyingOf(segments, periodic) : static_cast<std::size_t>(count);
    }
    return out;
}

Segments cut(const std::vector<glm::vec3> & points, const std::vector<float> & widths, const std::vector<std::int32_t> & vertices, bool cubic,
    const std::string & basis, bool periodic) {
    const bool perVarying = perVaryingValue(points.size(), widths.size(), vertices, varyingCount(vertices, cubic, basis, periodic));
    const std::uint32_t step = cubic ? stepOf(basis) : 1U;
    const std::size_t corners = cubic ? 4 : 2;
    const Matrix matrix = toBezier(basis);
    Segments out;
    std::size_t first = 0;
    std::size_t firstVarying = 0;
    for (std::size_t curve = 0; curve < vertices.size(); ++curve) {
        const auto count = static_cast<std::size_t>(vertices[curve]);
        const std::size_t segments = segmentsOf(vertices[curve], cubic, step, periodic);
        const std::size_t values = cubic ? varyingOf(segments, periodic) : count;
        for (std::size_t segment = 0; segment < segments; ++segment) {
            std::array<glm::dvec4, 4> given {};
            for (std::size_t corner = 0; corner < corners; ++corner) {
                const std::size_t at = first + (((segment * step) + corner) % count);
                given[corner] = glm::dvec4(glm::dvec3(points[at]), perVarying ? 0.0 : 0.5 * widths[at]);
            }
            std::optional<std::array<double, 2>> varying;
            if (perVarying) {
                varying = std::array<double, 2> { 0.5 * widths[firstVarying + segment], 0.5 * widths[firstVarying + ((segment + 1) % values)] };
            }
            if (cubic) {
                appendCubic(given, matrix, varying, out);
            } else {
                appendLinear(given, varying, out);
            }
            out.curves.push_back(static_cast<std::uint32_t>(curve));
            out.ends.push_back(!periodic && segment + 1 == segments ? 1U : 0U);
        }
        first += count;
        firstVarying += values;
    }
    return out;
}

};  // namespace v3d::moya::geometry
