/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Frustum.h"

namespace v3d::type::geometry {

namespace {

/**
 * A frustum plane is a combination of the matrix's rows: its w row times one weight plus
 * one of its x, y or z rows times another.
 *
 * glm is column major, so row i component k is m[k][i]. Reading m[i][k] instead extracts
 * the planes of the transposed matrix, which for anything but a symmetric one is a
 * different frustum.
 **/
Plane rowPlane(const glm::mat4& m, unsigned int row, float wWeight, float rowWeight) {
    Plane plane;
    for (unsigned int k = 0; k < 4; k++) {
        plane[k] = wWeight * m[k][3] + rowWeight * m[k][row];
    }
    return plane;
}

};  // namespace

/*
    the plane extraction is described in: http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf

    each plane is where one clip coordinate meets its bound, -w <= x <= w and the same in y.
    Depth is the one that differs: a [0, 1] clip volume keeps 0 <= z, so its near plane is the
    z row alone, and a [-1, 1] one keeps -w <= z, so its near plane is the w row plus the z
    row. Both keep z <= w, so their far planes agree.
*/
Frustum::Frustum(const glm::mat4& viewProjection, Depth depth) :
    planes_{
        rowPlane(viewProjection, 0, 1.0f, 1.0f),
        rowPlane(viewProjection, 0, 1.0f, -1.0f),
        rowPlane(viewProjection, 1, 1.0f, 1.0f),
        rowPlane(viewProjection, 1, 1.0f, -1.0f),
        rowPlane(viewProjection, 2, depth == Depth::ZeroToOne ? 0.0f : 1.0f, 1.0f),
        rowPlane(viewProjection, 2, 1.0f, -1.0f)
    } {
}

int Frustum::intersect(const AABBox& box) const {
    bool crossing = false;
    for (const Plane& plane : planes_) {
        const int hit = plane.classify(box);
        if (hit == Plane::OUTSIDE) {
            return OUTSIDE;
        }
        if (hit == Plane::CROSSING) {
            crossing = true;
        }
    }
    return crossing ? CROSSING : INSIDE;
}

const std::array<Plane, 6>& Frustum::planes() const noexcept {
    return planes_;
}

};  // namespace v3d::type::geometry
