/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/internal/Piece.h>
#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/texture/Options.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry::internal {

/**
 * Where one place of a cut surface moves: its displacement along its tangent, its bitangent and
 * its normal, and the tangent at its first sample in the surface's own space. The bitangent is
 * the normal's cross product with the tangent, times its handedness.
 **/
struct Lift {
    glm::vec3 along { 0.0F };
    glm::vec3 tangent { 1.0F, 0.0F, 0.0F };
    float handed { 1.0F };
};

/**
 * Each place's lift above a cut surface, from a material's displacement, run on the host through
 * a CPU device as a shading point would run it.
 *
 * Each place is shaded where it first lies among the triangles' corners, at the surface's first
 * sample. Its shading point lies where the placement puts it, with the limit's normal there as its
 * shading and geometric normal, so the program reads its position, normal and primvars as the
 * surface's own. Its tangent frame is the one Surfaces gives that point, from its `tangent` or its
 * `st`. The point has no footprint. The program's images are read through a texture cache of the
 * options, and its colours are converted into ACEScg by the default colour configuration.
 *
 * @param piece the surface, cut and undisplaced, as one geometry
 * @param displacement the material's displacement, of values
 * @param corners the place at each triangle's corners, three per triangle
 * @param normals the limit's normal at each place, three floats each, in the surface's own space
 * @param logger where the texture cache logs, or none for it to log through a logger of its own
 **/
std::vector<Lift> liftsOf(const Piece & piece, const material::Compiled & displacement, const std::vector<std::uint32_t> & corners,
    const std::vector<float> & normals, const glm::mat4 & placement, const texture::Options & options, const boost::shared_ptr<log::Logger> & logger);

/** Holds each lift no longer than a bound, along its own direction. True when one was longer. **/
bool heldWithin(std::vector<Lift> & lifts, float bound);

/**
 * The points of a cut surface's triangles' corners at one sample's points, each place raised by
 * its lift. The place moves along the sample's own limit normal, its first sample's tangent laid
 * flat against it, and the bitangent their cross product times its handedness. A place on a
 * shared edge takes its shared place's point after.
 **/
std::vector<glm::vec3> raised(const subdivision::Surface & surface, const subdivision::Tessellation & tessellation, const std::vector<float> & points,
    const std::vector<Lift> & lifts);

/**
 * What a displacement compiled to, as text: its instructions, its values, and the files and colour
 * spaces of its images, or nothing for none. Two cuts displaced by the same text are displaced
 * alike.
 **/
std::string fingerprintOf(const material::Compiled * displacement);

};  // namespace v3d::moya::geometry::internal
