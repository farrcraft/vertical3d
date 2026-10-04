/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <array>
#include <cassert>

#include "MicroPolygon.h"

namespace v3d::moya {

MicroPolygon::MicroPolygon() {
}

MicroPolygon::~MicroPolygon() {
}

Vertex & MicroPolygon::operator[] (unsigned int i) {
    assert(i < 4);
    return points_[i];
}

namespace {

/**
 * Twice the signed area of the triangle a, b, p in x and y.
 **/
float edge(const glm::vec3 & a, const glm::vec3 & b, const glm::vec2 & p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

bool triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec2 & p, float * depth) {
    const float area = edge(a, b, glm::vec2(c));
    // edge-on to the camera, or collapsed to a line by the projection: it covers nothing
    if (area == 0.0f) {
        return false;
    }
    // each weight is the sub-triangle opposite a corner, signed the way the whole one is, so
    // the point is inside when all three agree with it whichever way the triangle winds
    const float wa = edge(b, c, p) / area;
    const float wb = edge(c, a, p) / area;
    const float wc = edge(a, b, p) / area;
    if (wa < 0.0f || wb < 0.0f || wc < 0.0f) {
        return false;
    }
    *depth = wa * a.z + wb * b.z + wc * c.z;
    return true;
}

};  // namespace

bool covers(const std::array<glm::vec3, 4> & corners, const glm::vec2 & point, float * depth) {
    return triangle(corners[0], corners[1], corners[2], point, depth) ||
        triangle(corners[0], corners[2], corners[3], point, depth);
}
};  // namespace v3d::moya
