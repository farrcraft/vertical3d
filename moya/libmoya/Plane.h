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
        * distance to point
        */
        float distance(const glm::vec3 & point) const;
        int classify(const v3d::type::AABBox & aabb) const;
        int classify(const glm::vec3 & point) const;
        bool intersect(const glm::vec3 & start, const glm::vec3 & direction, glm::vec3 * hitPoint) const;
        bool intersectEdge(const glm::vec3 & A, const glm::vec3 & B, glm::vec3 * hitPoint) const;
        void clip(boost::shared_ptr<Polygon> poly);
        void normalize(void);

        float & operator[] (unsigned int i);

     private:
        glm::vec3 normal_;
        float distance_;
        float equation_[4];  // abcd
    };
};  // namespace v3d::moya
