/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "BRep.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::brep {

/**
 * Faces as a file holds them: a list of points, and each face as a run of indices into it.
 *
 * counts holds how many corners each face has, in order, and indices holds every face's
 * corners one face after another. A point may appear several times at the same position, as it
 * does where a file splits a vertex for its normals or texture coordinates.
 **/
struct Polygons {
    std::vector<glm::vec3> points;
    std::vector<std::uint32_t> counts;
    std::vector<std::uint32_t> indices;
};

/**
 * Which way a face's corners run, seen from outside.
 *
 * The mesh's own faces run clockwise. glTF and USD, by default, run counter-clockwise, and each
 * of their faces is reversed as it is built.
 **/
enum class Winding : std::uint8_t { Clockwise, CounterClockwise };

/**
 * How build() reads its input.
 *
 * joinTriangles joins pairs of triangles back into the quads they were cut from, for a format
 * that stores triangles only. Two triangles are joined across an edge they share when that edge
 * is the longest of each, as the diagonal of a quad is, when the quad they make is convex, and
 * when their normals differ by no more than joinAngle, in radians. Where a triangle could join
 * either of two neighbours, the flatter pair is joined first.
 **/
struct BuildOptions {
    /**
     * About 40 degrees, wide enough to join a quad that was not quite flat.
     **/
    static constexpr float DEFAULT_JOIN_ANGLE = 0.7F;

    Winding winding = Winding::Clockwise;
    bool joinTriangles = false;
    float joinAngle = DEFAULT_JOIN_ANGLE;
};

/**
 * What build() made.
 *
 * mesh is null when the input was refused, and problem then says why. skipped counts the faces
 * left out because they had fewer than three distinct corners, which files hold where a
 * triangle has collapsed to a line or a point.
 **/
struct Built {
    boost::shared_ptr<BRep> mesh;
    std::string problem;
    std::size_t skipped = 0;
};

/**
 * Build a mesh from polygons.
 *
 * Points at exactly the same position become one vertex, and -0 is the same position as 0. Two
 * corners in a row at one vertex count once. Faces that meet along an edge are paired across
 * it, and an edge with only one face is the mesh's boundary. Each face's normal is found from
 * its points. The time taken grows in proportion to the number of corners, apart from sorting
 * the pairs of triangles that could be joined.
 *
 * The input is refused when a count or an index is out of range, a point is not finite, a face
 * visits one vertex twice, or two faces run along one edge in the same direction. The last
 * happens where three or more faces meet at an edge, or where neighbouring faces disagree about
 * which side is outside.
 **/
Built build(const Polygons& polygons, const BuildOptions& options);

};  // namespace v3d::brep
