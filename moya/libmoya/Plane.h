/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../api/type/AABBox.h"

#include "Polygon.h"

namespace v3d::moya {
    /**
        *	defines a plane from one of:
        *		N dot P = D
        *		Ax+Bx+Cz = D
        */
    class Plane {
     public:
        Plane();
        Plane(const glm::vec3 & A, const glm::vec3 & B, const glm::vec3 & C);
        ~Plane();

        enum HalfSpace {
            NEGATIVE = -1,
            OUTSIDE = -1,
            ON_PLANE = 0,
            CROSSING = 0,
            POSITIVE = 1,
            INSIDE = 1
        };
        enum EquationIndex {
            A = 0,
            B = 1,
            C = 2,
            D = 3
        };
        void set(const glm::vec3 & n, float d);
        void calculate(const glm::vec3 & A, const glm::vec3 & B, const glm::vec3 & C);
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
        float distance(const glm::vec3 & point) const;
        int classify(const v3d::type::AABBox & aabb) const;
        int classify(const glm::vec3 & point) const;
        bool intersect(const glm::vec3 & start, const glm::vec3 & direction, glm::vec3 * hitPoint) const;
        bool intersectEdge(const glm::vec3 & A, const glm::vec3 & B, glm::vec3 * hitPoint) const;
        /**
        * Sutherland-Hodgman clip of a polygon against this plane, keeping the positive
        * half space. The polygon is rewritten in place, which is what lets a caller run one
        * plane after another over the same one.
        */
        void clip(const boost::shared_ptr<Polygon> & poly);
        void normalize(void);

        float & operator[] (unsigned int i);

     private:
        /*
            The equation is the plane's only state: normal in [A..C] and the negated distance
            from the origin in [D], so that distance(p) is the equation applied to p. A stored
            normal and distance alongside it is what the two writers that set only one of the
            pair used to disagree about.
        */
        float equation_[4] = { 0.0f, 0.0f, 0.0f, 0.0f };  // abcd
    };
};  // namespace v3d::moya
