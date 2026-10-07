/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include <glm/mat4x4.hpp>

#include "AABBox.h"
#include "Plane.h"

namespace v3d::type::geometry {

/**
 * The six planes bounding what a view projection matrix keeps, and a box tested against them.
 *
 * The planes come from the matrix by Gribb-Hartmann extraction, so the frustum is in whatever
 * space the matrix reads. A projection alone gives one in eye space. A projection times a
 * view gives one in world space. Each plane faces inward.
 **/
class Frustum final {
 public:
    /**
     * The depth range of the clip space the matrix builds, which decides where its near
     * plane is.
     **/
    enum class Depth {
        ZeroToOne,      // depth from 0 to 1, as Vulkan clips and every camera in api/type builds
        MinusOneToOne   // depth from -1 to 1, as glm::perspective and glm::ortho build by default
    };

    enum HitClassification {
        OUTSIDE = -1,
        CROSSING =  0,
        INSIDE =  1
    };

    /**
     * @param viewProjection the matrix whose clip volume this bounds
     * @param depth the depth range that matrix builds
     **/
    explicit Frustum(const glm::mat4& viewProjection, Depth depth = Depth::ZeroToOne);

    /**
     * Whether a box is wholly inside the frustum, wholly outside it, or crosses its edge.
     *
     * A box outside any one plane is outside, since the half spaces are intersected. A box
     * near a corner of the frustum can be outside it while inside every plane separately,
     * and is answered as crossing: the test never culls what it should keep.
     *
     * @param box the box, in the space the matrix reads
     **/
    int intersect(const AABBox& box) const;

    /**
     * The planes, in the order clip-space -x, +x, -y, +y, near, far. Clip y = -w is bounded
     * by plane 2, and clip y = w by plane 3. The cameras in api/type build a y-down clip
     * space, so for them plane 2 is the top of the screen and plane 3 the bottom.
     *
     * The planes are read straight out of the matrix and are not normalised. signedDistance()
     * on one tells the side a point is on, but its value is not a distance.
     **/
    const std::array<Plane, 6>& planes() const noexcept;

 private:
    std::array<Plane, 6> planes_;
};

};  // namespace v3d::type::geometry
