/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Plane.h"

#include <cmath>
#include <cassert>

namespace v3d::moya {


Plane::Plane() {
}

Plane::Plane(const glm::vec3 & A, const glm::vec3 & B, const glm::vec3 & C) {
    calculate(A, B, C);
}

Plane::~Plane() {
}

void Plane::normalize(void) {
    float mag;
    mag = std::sqrt(equation_[0] * equation_[0] + equation_[1] * equation_[1] + equation_[2] * equation_[2]);
    equation_[0] /= mag;
    equation_[1] /= mag;
    equation_[2] /= mag;
    equation_[3] /= mag;
}

void Plane::set(const glm::vec3 & n, float d) {
    equation_[0] = n[0];
    equation_[1] = n[1];
    equation_[2] = n[2];
    equation_[3] = -d;
}

glm::vec3 Plane::normal(void) const {
    return glm::vec3(equation_[0], equation_[1], equation_[2]);
}

float Plane::distance(void) const {
    return -equation_[3];
}

/*
    calculate the normal and distance of a plane given three coplanar points
    normal N is the cross product of vectors U and V:
        U = B - A
        V = C - A
        N.x = (U.y * V.z) - (U.z * V.y)
        N.y = (U.z * V.x) - (U.x * V.z)
        N.z = (U.x * V.y) - (U.y * V.x)
    the resulting normal will not be in normalized form
    distance d is the dot product of normalized normal N and point A:
        d = (N.x * A.x) + (N.y * A.y) + (N.z * A.z)
    plane equation:
        Ax + By + Cz = d where x, y, and z are points in 3d space, A, B, and C are
            are the x, y, and z components of the surface normal, and d is the
            distance value. any point (x,y,z) that satisfies this equation lies on
            the plane
*/
void Plane::calculate(const glm::vec3 & A, const glm::vec3 & B, const glm::vec3 & C) {
    glm::vec3 n = glm::normalize(glm::cross((B - A), (C - A)));  // normal = AB*AC
    set(n, glm::dot(n, A));
}

// tested - seems ok
void Plane::calculate(const glm::vec3 & normal, const glm::vec3 & point) {
    set(normal, glm::dot(normal, point));
}

float Plane::distance(const glm::vec3 & point) const {
    return equation_[0] * point[0] + equation_[1] * point[1] + equation_[2] * point[2] + equation_[3];
}

// tested - seems ok
// classifies whether a point is on either side of the plane or on the plane itself.
int Plane::classify(const glm::vec3 & point) const {
    /*
        Ax + By + Cz + D = 0
        Ax + By + Cz = -D
    */
    float dist = distance(point);

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
int Plane::classify(const v3d::type::AABBox & aabb) const {
    glm::vec3 corners[8];
    aabb.vertices(corners);
    int inside = 8;  // start with all 8 points inside
    for (unsigned int i = 0; i < 8; i++) {
        int side = classify(corners[i]);
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

float & Plane::operator[] (unsigned int i) {
    assert(i < 4);
    return equation_[i];
}


/* note - clipping happens after culling

    clip a single polygon to a single plane
    this is a 3D Sutherland-Hodgman Polygon Clipper 
    instead of clipping against a single clipping rectangle edge, we clip against
    a plane.
*/
void Plane::clip(const boost::shared_ptr<Polygon> & poly) {
    Polygon clippedPoly;
    Vertex s;
    Vertex p;
    Vertex i;
    glm::vec3 hit;
    size_t nverts;

    nverts = poly->vertexCount();
    // fewer than three vertices bound no area to keep, and the walk below opens on the vertex
    // before the first one
    if (nverts < 3) {
        return;
    }
    s = poly->vertex(nverts - 1);  // start with last vertex
    for (size_t j = 0; j < nverts; j++) {
        p = poly->vertex(j);
        /*
         there are 4 possible test cases:
            case 1: s & p both inside	 - in/in
            case 2: s inside, p outside - in/out
            case 3: s & p both outside	 - out/out
            case 4: s outside, p inside - out/in
         */
        int side = classify(p.point());
        if (side == POSITIVE || side == ON_PLANE) {  // cases 1 & 4
             side = classify(s.point());
             if (side == POSITIVE || side == ON_PLANE) {
                  // case 1
                  clippedPoly.addVertex(p);
             } else {
                  // case 4
                  intersectEdge(s.point(), p.point(), &hit);
                  i.point(hit);
                  clippedPoly.addVertex(i);
                  clippedPoly.addVertex(p);
             }
        } else {  // cases 2 & 3
             side = classify(s.point());
             if (side == POSITIVE || side == ON_PLANE) {
                  // case 2
                  intersectEdge(s.point(), p.point(), &hit);
                  i.point(hit);
                  clippedPoly.addVertex(i);
             } else {
                  // case 3
                  // entire edge is clipped - no action required
             }
        }
        s = p;
    }

    poly->clear();
    for (size_t k = 0; k < clippedPoly.vertexCount(); k++) {
        poly->addVertex(clippedPoly.vertex(k));
    }
}

// ray intersection test
bool Plane::intersect(const glm::vec3 & start, const glm::vec3 & direction, glm::vec3 * hitPoint) const {
    float denom = glm::dot(normal(), direction);
    if (denom == 0.0) {  // ray and plane are parallel
        return false;
    }
    float tval = (distance() - (glm::dot(normal(), start))) / denom;
    if (tval >= 0.0) {  // intersection isn't behind ray
        glm::vec3 hit = start + direction * tval;
        hitPoint->x = hit.x;
        hitPoint->y = hit.y;
        hitPoint->z = hit.z;
        return true;
    }
    return false;
}

// tested - seems ok
bool Plane::intersectEdge(const glm::vec3 & A, const glm::vec3 & B, glm::vec3 * hitPoint) const {
    glm::vec3 direction = B - A;
    float denom = glm::dot(normal(), direction);
    if (denom == 0.0) {
        return false;
    }
    float tval = (distance() - (glm::dot(normal(), A))) / denom;
    if (tval >= 0.0 && tval <= 1.0) {
        glm::vec3 hit = A + (direction * tval);
        hitPoint->x = hit.x;
        hitPoint->y = hit.y;
        hitPoint->z = hit.z;
        return true;
    }
    return false;
}

};  // namespace v3d::moya
