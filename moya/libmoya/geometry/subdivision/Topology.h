/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::moya::geometry::subdivision {

/** How a subdivision surface is refined: by Catmull-Clark, Loop or bilinear subdivision. **/
enum class Scheme : std::uint8_t {
    CatmullClark,
    Loop,
    Bilinear
};

/**
 * Which boundary vertices a subdivision surface interpolates, as RI's interpolateboundary names
 * them: none, so that its boundary faces are left out, its boundary edges, or its edges and the
 * corners where two meet.
 **/
enum class Boundary : std::uint8_t {
    None,
    Edges,
    EdgesAndCorners
};

/**
 * A subdivision surface's control mesh and its tags, as the scene's subdivision holds them. A
 * face's vertices are a run of the indices, as a mesh's are.
 **/
struct Topology {
    std::vector<std::int32_t> vertices;
    std::vector<std::int32_t> indices;

    /** How many points the indices reach into. **/
    std::uint32_t points { 0 };

    Scheme scheme { Scheme::CatmullClark };

    /** The vertices of each crease in turn, how many each has, and one sharpness each. **/
    std::vector<std::int32_t> creases;
    std::vector<std::int32_t> creaseLengths;
    std::vector<float> creaseSharpness;

    /** The vertices that are corners, with one sharpness each. **/
    std::vector<std::int32_t> corners;
    std::vector<float> cornerSharpness;

    /** The faces left open. **/
    std::vector<std::int32_t> holes;

    Boundary boundary { Boundary::None };
};

};  // namespace v3d::moya::geometry::subdivision
