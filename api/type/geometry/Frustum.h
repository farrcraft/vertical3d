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
 * space the matrix reads: a projection alone gives one in eye space, and a projection times a
 * view gives one in world space. Each plane faces inward.
 **/
class Frustum final {
 public:
    /**
     * The depth range of the clip space the matrix builds, which decides where its near
     * plane is - ADR-0024.
     **/
    enum class Depth {
        ZeroToOne,      // Vulkan, and every camera in api/type
        MinusOneToOne   // OpenGL, and moya's RenderContext
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
     * The planes, in the order left, right, bottom, top, near, far.
     **/
    const std::array<Plane, 6>& planes() const noexcept;

 private:
    std::array<Plane, 6> planes_;
};

};  // namespace v3d::type::geometry
