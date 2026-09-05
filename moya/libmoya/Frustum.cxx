/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Frustum.h"

#include <map>
#include <string>

namespace v3d::moya {

namespace {

    /**
     * A frustum plane is the matrix's w row plus or minus one of its x, y or z rows.
     *
     * glm is column major, so row i component k is m[k][i]. Reading m[i][k] instead extracts
     * the planes of the transposed matrix, which for anything but a symmetric one is a
     * different frustum.
     **/
    Plane rowPlane(const glm::mat4x4 & m, unsigned int row, float sign) {
        Plane plane;
        for (unsigned int k = 0; k < 4; k++) {
            plane[k] = m[k][3] + sign * m[k][row];
        }
        return plane;
    }

};  // namespace

Frustum::Frustum() {
}

Frustum::Frustum(const glm::mat4x4 & projection) {
    extract(projection);
}

Frustum::~Frustum() {
}

/*
    the plane extraction is described in: http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf

    the near plane is the w row plus the z row rather than the z row alone. The z-row-only form
    is for a clip volume whose depth runs [0, 1]; both glm and RenderContext::projection build
    one running [-1, 1], which the far plane below was already written for.
*/
void Frustum::extract(const glm::mat4x4 & projection) {
    _clippingPlanes["left"] = rowPlane(projection, 0, 1.0f);
    _clippingPlanes["right"] = rowPlane(projection, 0, -1.0f);
    _clippingPlanes["bottom"] = rowPlane(projection, 1, 1.0f);
    _clippingPlanes["top"] = rowPlane(projection, 1, -1.0f);
    _clippingPlanes["near"] = rowPlane(projection, 2, 1.0f);
    _clippingPlanes["far"] = rowPlane(projection, 2, -1.0f);
}

void Frustum::normalize(void) {
    std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
    for (; it != _clippingPlanes.end(); it++) {
        (it->second).normalize();
    }
}

/*
    tests aabb against all of the frustum's clipping planes 
    aabb is either inside, outside, or intersecting the frustum
*/
int Frustum::intersect(const v3d::type::AABBox & aabb) {
    // one plane excluding the box excludes it from the frustum: the half spaces are
    // intersected, not unioned, so a box outside any one of them is outside all six
    bool crossing = false;
    std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
    for (; it != _clippingPlanes.end(); it++) {
        int hit = (it->second).classify(aabb);
        if (hit == Plane::OUTSIDE) {
            return OUTSIDE;
        }
        if (hit == Plane::CROSSING) {
            crossing = true;
        }
    }
    return crossing ? CROSSING : INSIDE;
}

// clip a polygon against each of the clipping planes in the frustum
void Frustum::clip(const boost::shared_ptr<Polygon> & poly) {
    std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
    for (; it != _clippingPlanes.end(); it++) {
        (it->second).clip(poly);
    }
}

};  // namespace v3d::moya
