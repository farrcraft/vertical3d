/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::brep {

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
 * that stores triangles only. Two triangles are joined across an edge they share when three
 * things hold. The edge is the longest of each, as the diagonal of a quad is. The quad they
 * make is convex. Their normals differ by no more than joinAngle, in radians. Where a triangle
 * could join either of two neighbours, the flatter pair is joined first.
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

};  // namespace v3d::brep
