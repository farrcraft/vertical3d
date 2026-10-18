/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <glm/glm.hpp>

namespace v3d::type::geometry {

/**
 * An axis aligned rectangle: a top left corner and a size.
 */
class Bound2D {
 public:
    /**
     * The top left corner of a bounded area, and its width and height.
     **/
    struct Area {
        glm::vec2 position{0.0F};
        glm::vec2 size{0.0F};
    };

    explicit Bound2D(const Area & area);

    /**
        * Get the size of the bounded area
        *
        * @return the width and height of the bounded area
        */
    glm::vec2 size() const;
    /**
        * Get the position of the bounded area
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
