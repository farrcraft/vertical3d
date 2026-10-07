/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <algorithm>

#include <glm/glm.hpp>

// #include "Polygon.h"

namespace v3d::type::geometry {
/**
 * A class defining a 3D Axis Aligned Bounding Box.
 */
class AABBox final {
 public:
    AABBox();
    ~AABBox();

    /**
        * Get the minimum extents of the bounding box.
        * @return a vector of the minimum extents of the box.
        */
    glm::vec3 min() const;
    /**
        * Get the maximum extents of the bounding box.
        * @return a vector of the maximum extents of the box.
        */
    glm::vec3 max() const;
    /**
        * Get the 8 vertices that compose the bounding box
        * @param verts an array of Vector3's to hold the 8 vertices.
        */
    void vertices(glm::vec3 * verts) const;
    /**
        * Set the minimum extents of the bounding box.
        * @param v the vector containing the minimum extents.
        */
    void min(const glm::vec3 & v);
    /**
        * Set the maximum extents of the bounding box.
        * @param v the vector containing the maximum extents.
        */
    void max(const glm::vec3 & v);
    /**
        * Set the minimum and maximum extents of the bounding box.
        * @param min the minimum extents of the bounding box.
        * @param max the maximum extents of the bounding box.
        */
    void extents(const glm::vec3 & min, const glm::vec3 & max);

    /**
        * Extend bounds to include a point.
        * If the point is already within the bounding box then there is no
        * change to the bounding box. The point and bounding box are 
        * assumed to exist in the same coordinate space.
        * @param point the point to include in the bounding box.
        */
    void extend(const glm::vec3 & point);

    /**
        * Whether a point is inside the box. The faces count as inside.
        * @param point the point to test, in the box's own space.
        */
    bool contains(const glm::vec3 & point) const;
    /**
        * Whether two boxes share any volume. The faces count as inside, so two boxes
        * that meet at a face overlap.
        * @param other the box to test against, in the same space.
        */
    bool overlaps(const AABBox & other) const;

    // set min & max to bound polygon
    // void bound(const Polygon & poly);

 private:
    glm::vec3 min_;
    glm::vec3 max_;
};

};  // namespace v3d::type::geometry
