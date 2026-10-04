/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include "Vertex.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya {
// a flat shaded quadrilateral with an area of about 1/4 of a pixel
class MicroPolygon {
 public:
    MicroPolygon();
    ~MicroPolygon();

    Vertex & operator[] (unsigned int i);

 private:
    Vertex points_[4];
};

/**
 * Whether a point in raster space falls inside a micropolygon, and its depth there.
 *
 * The micropolygon is its four corners in raster space, in the order
 * MicroPolygonGrid::microPolygon gives them, and is tested as the two triangles 0, 1, 2 and
 * 0, 2, 3 - a micropolygon need not be planar, and a triangle always is. Either winding is
 * inside, and a point on an edge is too, so two micropolygons sharing an edge leave no gap.
 *
 * @param depth set to the corners' depths interpolated across the triangle the point is in
 **/
bool covers(const std::array<glm::vec3, 4> & corners, const glm::vec2 & point, float * depth);
};  // namespace v3d::moya
