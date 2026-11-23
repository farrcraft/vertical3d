/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Primvars.h>

#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/scene/Declaration.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::moya::geometry::internal {

namespace {

/** Refuses values of another count than a class gives. **/
void expect(const std::vector<float> & values, std::size_t width, std::size_t elements, scene::Interpolation interpolation) {
    if (values.size() != elements * width) {
        throw std::invalid_argument(std::string("a ") + scene::name(interpolation) + " primvar of " + std::to_string(values.size() / width) +
            " values where it needs " + std::to_string(elements));
    }
}

/** Appends the element-th value of width floats. **/
void append(std::vector<float> & out, const std::vector<float> & values, std::size_t width, std::size_t element) {
    const auto from = values.begin() + static_cast<std::ptrdiff_t>(element * width);
    out.insert(out.end(), from, from + static_cast<std::ptrdiff_t>(width));
}

/** The one value of a constant primvar. **/
Spread constant(const std::vector<float> & values, std::size_t width) {
    expect(values, width, 1, scene::Interpolation::Constant);
    return Spread { .perPrimitive = 0, .values = values };
}

};  // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the mesh's counts of points, then of faces
Spread spreadOverMesh(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width, const Triangles & triangles,
    const std::vector<std::int32_t> & indices, std::size_t points, std::size_t faces) {
    Spread out;
    switch (interpolation) {
        case scene::Interpolation::Constant:
            return constant(values, width);
        case scene::Interpolation::Uniform: {
            expect(values, width, faces, interpolation);
            out.perPrimitive = 1;
            for (const std::uint32_t face : triangles.faces) {
                append(out.values, values, width, face);
            }
            return out;
        }
        case scene::Interpolation::Varying:
        case scene::Interpolation::Vertex:
            expect(values, width, points, interpolation);
            out.perPrimitive = 3;
            for (const std::uint32_t corner : triangles.corners) {
                append(out.values, values, width, static_cast<std::size_t>(indices[corner]));
            }
            return out;
        case scene::Interpolation::FaceVarying:
            expect(values, width, indices.size(), interpolation);
            out.perPrimitive = 3;
            for (const std::uint32_t corner : triangles.corners) {
                append(out.values, values, width, corner);
            }
            return out;
    }
    return out;
}

Spread spreadOverSurface(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width,
    const subdivision::Surface & surface, const subdivision::Tessellation & tessellation) {
    if (interpolation == scene::Interpolation::Constant) {
        return constant(values, width);
    }
    const std::vector<float> places = surface.at(values, width, interpolation, tessellation);
    Spread out { .perPrimitive = 3, .values = {} };
    out.values.reserve(tessellation.corners.size() * width);
    for (const std::uint32_t corner : tessellation.corners) {
        append(out.values, places, width, corner);
    }
    return out;
}

Spread spreadOverPoints(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width, std::size_t points) {
    switch (interpolation) {
        case scene::Interpolation::Constant:
        case scene::Interpolation::Uniform:
            return constant(values, width);
        case scene::Interpolation::Varying:
        case scene::Interpolation::Vertex:
            expect(values, width, points, interpolation);
            return Spread { .perPrimitive = 1, .values = values };
        case scene::Interpolation::FaceVarying:
            break;
    }
    throw std::invalid_argument("a facevarying primvar on points, which have no faces");
}

Spread spreadOverQuadric(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width) {
    if (interpolation == scene::Interpolation::Constant || interpolation == scene::Interpolation::Uniform) {
        return constant(values, width);
    }
    throw std::invalid_argument(std::string("a ") + scene::name(interpolation) + " primvar on a quadric, which moya does not yet read");
}

Spread spreadOverCurves(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width,
    const std::vector<std::uint32_t> & curves, std::size_t curveCount) {
    if (interpolation == scene::Interpolation::Constant) {
        return constant(values, width);
    }
    if (interpolation != scene::Interpolation::Uniform) {
        throw std::invalid_argument(std::string("a ") + scene::name(interpolation) + " primvar on curves, which moya does not yet read");
    }
    expect(values, width, curveCount, interpolation);
    Spread out { .perPrimitive = 1, .values = {} };
    for (const std::uint32_t curve : curves) {
        append(out.values, values, width, curve);
    }
    return out;
}

};  // namespace v3d::moya::geometry::internal
