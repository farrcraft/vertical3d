/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>

namespace v3d::type::geometry {

/**
 * A 2 dimensional clipped bounding plane object
 */
class Bound2D {
 public:
    /**
        * Construct a new 2D bounding object
        *
        * @param x the left position of the clipping plane
        * @param y the top position of the clipping plane
        * @param width the width of the bound
        * @param height the height of the bound
        */
    Bound2D(float x, float y, float width, float height);
    /**
        * Construct a new 2D bounding object
        *
        * @param position the top left corner of the clipping plane
        * @param size the width and height of the bounded area
        */
    Bound2D(const glm::vec2 & position, const glm::vec2 & size);

    /**
        * Get the size of the bounding volume
        *
        * @return the width and height of the bounded area
        */
    glm::vec2 size() const;
    /**
        * Get the position of the bounding volume
        *
        * @return the top left corner of the bounding box
        */
    glm::vec2 position() const;
    /**
        * Whether a point is inside the bound. The edge counts as inside.
        *
        * @param point the point to test
        */
    bool contains(const glm::vec2 & point) const;
    /**
        * Whether two bounds share any area. The edge counts as inside, so two bounds that
        * meet along an edge overlap.
        *
        * @param other the bound to test against
        */
    bool overlaps(const Bound2D & other) const;

 private:
    glm::vec2 size_;
    glm::vec2 position_;
};

};  // namespace v3d::type::geometry
