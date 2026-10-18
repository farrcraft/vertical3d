/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <array>
#include <cassert>

#include "MicroPolygon.h"

namespace v3d::moya::reyes {

MicroPolygon::MicroPolygon() {
}

MicroPolygon::~MicroPolygon() {
}

Vertex & MicroPolygon::operator[] (unsigned int index) {
    assert(index < 4);
    return points_[index];
}

namespace {

/**
 * Twice the signed area of the triangle from, to, point in x and y.
 **/
float edge(const glm::vec3 & from, const glm::vec3 & to, const glm::vec2 & point) {
    return ((to.x - from.x) * (point.y - from.y)) - ((to.y - from.y) * (point.x - from.x));
}

bool triangle(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2, const glm::vec2 & point, float * depth) {
    const float area = edge(p0, p1, glm::vec2(p2));
    // edge-on to the camera, or collapsed to a line by the projection: it covers nothing
    if (area == 0.0F) {
        return false;
    }
    // each weight is the sub-triangle opposite a corner, signed the way the whole one is, so
    // the point is inside when all three agree with it whichever way the triangle winds
    const float wa = edge(p1, p2, point) / area;
    const float wb = edge(p2, p0, point) / area;
    const float wc = edge(p0, p1, point) / area;
    if (wa < 0.0F || wb < 0.0F || wc < 0.0F) {
        return false;
    }
    *depth = (wa * p0.z) + (wb * p1.z) + (wc * p2.z);
    return true;
}

};  // namespace

bool covers(const std::array<glm::vec3, 4> & corners, const glm::vec2 & point, float * depth) {
    return triangle(corners[0], corners[1], corners[2], point, depth) ||
        triangle(corners[0], corners[2], corners[3], point, depth);
}
};  // namespace v3d::moya::reyes
