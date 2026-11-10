/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/scene/Declaration.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace v3d::moya::geometry::internal {

/**
 * A primvar's values laid out as shading reads them: one value for the whole geometry when it
 * holds none per primitive, or that many for each primitive.
 **/
struct Spread {
    std::uint32_t perPrimitive { 0 };
    std::vector<float> values;
};

/**
 * A mesh's primvar spread over its triangles. A constant value is the mesh's, a uniform one is
 * each triangle's face's, and a varying, vertex or face-varying one is read at each corner.
 *
 * @throws std::invalid_argument when the values are of another count than the class gives
 **/
Spread spreadOverMesh(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width, const Triangles & triangles,
    const std::vector<std::int32_t> & indices, std::size_t points, std::size_t faces);

/**
 * A subdivision surface's primvar spread over the triangles of a tessellation, as the surface
 * evaluates it at each place. A constant value is the surface's.
 *
 * @throws std::invalid_argument when the values are of another count than the class gives
 **/
Spread spreadOverSurface(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width,
    const subdivision::Surface & surface, const subdivision::Tessellation & tessellation);

/**
 * A point cloud's primvar: a constant or uniform value is the cloud's, and a varying or vertex
 * one is each point's.
 *
 * @throws std::invalid_argument when the values are of another count than the class gives, or
 *         for a face-varying primvar, which a point cloud has no faces for
 **/
Spread spreadOverPoints(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width, std::size_t points);

/**
 * A quadric's primvar, which is constant or uniform: one value for the whole quadric.
 *
 * @throws std::invalid_argument when the values are of another count than one, or for a varying,
 *         vertex or face-varying primvar, which a quadric does not yet hold
 **/
Spread spreadOverQuadric(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width);

/**
 * Curves' primvar spread over their segments: a constant value is the curves', and a uniform one
 * is each segment's curve's.
 *
 * @param curves the curve each segment was cut from
 * @throws std::invalid_argument when the values are of another count than the class gives, or for
 *         a varying, vertex or face-varying primvar, which curves do not yet hold
 **/
Spread spreadOverCurves(scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width,
    const std::vector<std::uint32_t> & curves, std::size_t curveCount);

};  // namespace v3d::moya::geometry::internal
