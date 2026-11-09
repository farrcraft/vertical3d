/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/camera/Camera.h>

#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

/**
 * How finely a surface is cut to be seen through a dicing camera: each edge into pieces no longer
 * than an edge length in pixels, where a placement puts the surface.
 *
 * A piece's length in pixels is its length over the width of a pixel at its middle's depth, as
 * Cycles measures it. An edge seen end on is then cut as finely as one seen across. A pixel is the
 * narrower of its width and height on the screen. A perspective camera's pixel widens with depth,
 * and is measured at a point behind the camera by its distance from the camera's plane.
 **/
class Dicing final {
 public:
    /** The most pieces an edge is cut into, however near the camera it lies. **/
    static constexpr std::uint32_t kMostPieces = 64;

    /** A measure from a camera, of an edge length in pixels, through a placement in the world. **/
    Dicing(const kernel::camera::Camera & camera, float edgeLength, const glm::mat4 & placement);

    /**
     * The pieces an edge is cut into, from points along it in the surface's own space: from one to
     * kMostPieces. An edge length of zero or less cuts every edge into kMostPieces.
     **/
    std::uint32_t operator()(const std::vector<glm::vec3> & along) const;

 private:
    /** The surface's own space to the camera's. **/
    glm::mat4 toCamera_;
    bool perspective_;

    /** A pixel's width in camera space: at a depth of one for a perspective camera. **/
    float pixel_;
    float edgeLength_;
};

};  // namespace v3d::moya::geometry
