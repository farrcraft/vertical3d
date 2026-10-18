/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Plane.h"

#include "AABBox.h"

#include <cmath>
#include <cassert>

#include <glm/geometric.hpp>

namespace v3d::type::geometry {

Plane::Plane() {
}

Plane::Plane(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2) {
    calculate(p0, p1, p2);
}

Plane::~Plane() {
}

void Plane::normalize(void) {
    float mag;
    mag = std::sqrt((equation_[0] * equation_[0]) + (equation_[1] * equation_[1]) + (equation_[2] * equation_[2]));
    equation_[0] /= mag;
    equation_[1] /= mag;
    equation_[2] /= mag;
    equation_[3] /= mag;
}

void Plane::set(const glm::vec3 & n, float originDistance) {
    equation_[0] = n[0];
    equation_[1] = n[1];
    equation_[2] = n[2];
    equation_[3] = -originDistance;
}

glm::vec3 Plane::normal(void) const {
    return glm::vec3(equation_[0], equation_[1], equation_[2]);
}

float Plane::distance(void) const {
    return -equation_[3];
}

/*
    Calculate the normal and distance of a plane given three coplanar points.

    The normal N is the cross product of vectors U and V:
        U = p1 - p0
        V = p2 - p0
        N.x = (U.y * V.z) - (U.z * V.y)
        N.y = (U.z * V.x) - (U.x * V.z)
        N.z = (U.x * V.y) - (U.y * V.x)

    The resulting normal is not in normalized form.

    The distance d is the dot product of the normalized normal N and point p0:
        d = (N.x * p0.x) + (N.y * p0.y) + (N.z * p0.z)

    In the plane equation Ax + By + Cz = d, x, y and z are a point in 3d space. A, B and C
    are the x, y and z components of the surface normal, and d is the distance value. Any
    point (x, y, z) that satisfies this equation lies on the plane.
*/
void Plane::calculate(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2) {
    const glm::vec3 planeNormal = glm::normalize(glm::cross((p1 - p0), (p2 - p0)));  // normal = p0p1*p0p2
    set(planeNormal, glm::dot(planeNormal, p0));
}

void Plane::calculate(const glm::vec3 & normal, const glm::vec3 & point) {
    set(normal, glm::dot(normal, point));
}

float Plane::signedDistance(const glm::vec3 & point) const {
    return (equation_[0] * point[0]) + (equation_[1] * point[1]) + (equation_[2] * point[2]) + equation_[3];
}

// classifies whether a point is on either side of the plane or on the plane itself.
int Plane::classify(const glm::vec3 & point) const {
    /*
        Ax + By + Cz + D = 0
        Ax + By + Cz = -D
    */
    const float dist = signedDistance(point);

    if (dist < 0.0) {
        return NEGATIVE;
    }
    if (dist > 0.0) {
        return POSITIVE;
    }
    return ON_PLANE;
}

/*
    classify the 8 points of the aabb against the plane
    see: http://www.flipcode.com/articles/article_frustumculling.shtml
*/
int Plane::classify(const AABBox & aabb) const {
    glm::vec3 corners[8];
    aabb.vertices(corners);
    int inside = 8;  // start with all 8 points inside
    for (unsigned int i = 0; i < 8; i++) {
        const int side = classify(corners[i]);
        if (side == NEGATIVE) {
            inside--;
        }
    }
    if (inside == 0) {  // all points were outside
        return OUTSIDE;
    }
    if (inside == 8) {  // all points were inside
        return INSIDE;
    }
    // points were on both sides so box intersects plane
    return CROSSING;
}

float & Plane::operator[] (unsigned int index) {
    assert(index < 4);
    return equation_[index];
}

float Plane::operator[] (unsigned int index) const {
    assert(index < 4);
    return equation_[index];
}


bool Plane::intersectEdge(const glm::vec3 & start, const glm::vec3 & end, glm::vec3 * hitPoint) const {
    const glm::vec3 direction = end - start;
    const float denom = glm::dot(normal(), direction);
    if (denom == 0.0) {
        return false;
    }
    const float tval = (distance() - (glm::dot(normal(), start))) / denom;
    if (tval >= 0.0 && tval <= 1.0) {
        const glm::vec3 hit = start + (direction * tval);
        hitPoint->x = hit.x;
        hitPoint->y = hit.y;
        hitPoint->z = hit.z;
        return true;
    }
    return false;
}

};  // namespace v3d::type::geometry
