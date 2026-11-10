/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/surface/Primvar.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry::internal {

/**
 * What one gather made of one geometry, held apart from every other geometry so that a later
 * gather can place it again. The geometry's offsets count from the start of the piece's own
 * values.
 **/
struct Piece {
    Geometry geometry;

    std::vector<glm::vec3> points;
    std::vector<float> widths;
    std::vector<kernel::geometry::primitive::Quadric> quadrics;
    std::vector<glm::vec4> controls;
    std::vector<std::uint32_t> ends;
    std::vector<float> times;
    std::vector<std::uint32_t> faces;

    /** Its primvars, each counting its values from the start of the piece's own. **/
    std::vector<kernel::surface::Primvar> primvars;
    std::vector<float> primvarValues;
    std::vector<std::string> primvarSpaces;

    /** What was reported while it was made, reported again where it is placed again. **/
    std::vector<std::string> reports;

    /** A subdivision surface's limit, refined once from its topology. **/
    std::optional<subdivision::Surface> surface;

    /** What a subdivision surface was measured from to be cut, which a later cut has to match. **/
    std::vector<float> dicing;

    /** Whether the piece holds a geometry, which a subdivision surface does once it is cut. **/
    bool made { false };
};

};  // namespace v3d::moya::geometry::internal
