/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include "AABBox.h"

#include <glm/vec3.hpp>

namespace v3d::type::geometry {

/**
    *	A plane: the points P where N dot P = d, that is Ax + By + Cz = d.
    *	The equation is stored as A, B, C and D = -d, so Ax + By + Cz + D = 0.
    */
class Plane {
 public:
    Plane();
    Plane(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2);
    ~Plane();

    enum HalfSpace : std::int8_t {
        NEGATIVE = -1,
        OUTSIDE = -1,
        ON_PLANE = 0,
        CROSSING = 0,
        POSITIVE = 1,
        INSIDE = 1
    };
    enum EquationIndex : std::uint8_t {
        A = 0,
        B = 1,
        C = 2,
        D = 3
    };
    void set(const glm::vec3 & n, float originDistance);
    void calculate(const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2);
    void calculate(const glm::vec3 & normal, const glm::vec3 & point);

    /**
    * The plane's normal, which is normalized only if the plane is.
    */
    glm::vec3 normal(void) const;
    /**
    * The plane's distance from the origin along its normal.
    */
    float distance(void) const;

    /**
    * signed distance to point
    */
    float signedDistance(const glm::vec3 & point) const;
    int classify(const AABBox & aabb) const;
    int classify(const glm::vec3 & point) const;
    bool intersectEdge(const glm::vec3 & start, const glm::vec3 & end, glm::vec3 * hitPoint) const;
    void normalize(void);

    float & operator[] (unsigned int index);
    float operator[] (unsigned int index) const;

 private:
    /*
        The equation is the plane's only state: the normal in [A..C] and the negated
        distance from the origin in [D]. signedDistance(p) is the equation applied to p.
        A separately stored normal and distance could disagree with it.
    */
    float equation_[4] = { 0.0F, 0.0F, 0.0F, 0.0F };  // abcd
};
};  // namespace v3d::type::geometry
