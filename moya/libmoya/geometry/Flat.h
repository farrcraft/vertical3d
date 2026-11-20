/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/PlacedShapes.h>
#include <moya/libmoya/kernel/geometry/Placement.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry {

/**
 * Every primitive of a scene placed, in host memory, as kernel::geometry::Placed holds it on a
 * device.
 **/
struct Flat {
    std::vector<glm::vec3> points;
    std::vector<std::uint32_t> faces;
    std::vector<kernel::geometry::Placement> placements;

    /** The placement each triangle belongs to, one per triangle. **/
    std::vector<std::uint32_t> runs;

    std::vector<kernel::geometry::PlacedShapes> shapes;
    std::vector<glm::vec4> spheres;
    std::vector<kernel::geometry::primitive::Quadric> quadrics;
    std::vector<glm::vec4> controls;
    std::vector<std::uint32_t> curves;
    std::vector<std::uint32_t> ends;

    /** What was left out and why, once each, as the log reports it. **/
    std::vector<std::string> reports;
};

};  // namespace v3d::moya::geometry
