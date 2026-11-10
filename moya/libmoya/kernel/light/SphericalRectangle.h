/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/** Half a turn, in radians. **/
inline constexpr float kHalfTurn = 3.14159265358979323846F;

/**
 * A rectangle as a point sees it, sampled uniformly in solid angle by Ureña, Fajardo and King's
 * method. The rectangle has a corner and two edges at right angles. In the frame of the edges,
 * with the point at the origin and z pointing away from the rectangle, it spans x0 to x1, y0 to y1
 * at a height z0 below zero. b0 and b1 are the z of the normals of the planes through the point
 * and its first and third edges, and lastAngles the sum of its last two inner angles, which
 * sample() reads.
 **/
struct SphericalRectangle {
    glm::vec3 origin { 0.0F };
    glm::vec3 xAxis { 1.0F, 0.0F, 0.0F };
    glm::vec3 yAxis { 0.0F, 1.0F, 0.0F };
    glm::vec3 zAxis { 0.0F, 0.0F, 1.0F };
    float x0 { 0.0F };
    float x1 { 0.0F };
    float y0 { 0.0F };
    float y1 { 0.0F };
    float z0 { 0.0F };
    float b0 { 0.0F };
    float b1 { 0.0F };
    float lastAngles { 0.0F };
    float solidAngle { 0.0F };

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the rectangle
    MOYA_KERNEL static SphericalRectangle seen(const glm::vec3 & from, const glm::vec3 & corner, const glm::vec3 & edgeX, const glm::vec3 & edgeY) {
        SphericalRectangle out;
        out.origin = from;
        const float lengthX = glm::length(edgeX);
        const float lengthY = glm::length(edgeY);
        out.xAxis = edgeX / lengthX;
        out.yAxis = edgeY / lengthY;
        out.zAxis = glm::cross(out.xAxis, out.yAxis);
        const glm::vec3 offset = corner - from;
        out.z0 = glm::dot(offset, out.zAxis);
        // z points away from the rectangle
        if (out.z0 > 0.0F) {
            out.zAxis = -out.zAxis;
            out.z0 = -out.z0;
        }
        out.x0 = glm::dot(offset, out.xAxis);
        out.y0 = glm::dot(offset, out.yAxis);
        out.x1 = out.x0 + lengthX;
        out.y1 = out.y0 + lengthY;
        const glm::vec3 v00 { out.x0, out.y0, out.z0 };
        const glm::vec3 v01 { out.x0, out.y1, out.z0 };
        const glm::vec3 v10 { out.x1, out.y0, out.z0 };
        const glm::vec3 v11 { out.x1, out.y1, out.z0 };
        const glm::vec3 n0 = glm::normalize(glm::cross(v00, v10));
        const glm::vec3 n1 = glm::normalize(glm::cross(v10, v11));
        const glm::vec3 n2 = glm::normalize(glm::cross(v11, v01));
        const glm::vec3 n3 = glm::normalize(glm::cross(v01, v00));
        const float g0 = angleBetween(-n0, n1);
        const float g1 = angleBetween(-n1, n2);
        const float g2 = angleBetween(-n2, n3);
        const float g3 = angleBetween(-n3, n0);
        constexpr float kWholeTurn = 6.28318530717958647692F;
        out.b0 = n0.z;
        out.b1 = n2.z;
        out.lastAngles = g2 + g3;
        out.solidAngle = g0 + g1 + g2 + g3 - kWholeTurn;
        return out;
    }

    /** A point of the rectangle, drawn uniformly in the solid angle it subtends. **/
    MOYA_KERNEL glm::vec3 sample(const glm::vec2 & u) const {
        constexpr float kBelowOne = 0.99999994F;
        const float au = (u.x * solidAngle) - lastAngles;
        const float fu = ((std::cos(au) * b0) - b1) / std::sin(au);
        float cu = std::copysign(1.0F / std::sqrt((fu * fu) + (b0 * b0)), fu);
        cu = std::fmax(-kBelowOne, std::fmin(kBelowOne, cu));
        float xu = -(cu * z0) / std::sqrt(std::fmax(0.0F, 1.0F - (cu * cu)));
        xu = std::fmax(x0, std::fmin(x1, xu));
        const float reach = std::sqrt((xu * xu) + (z0 * z0));
        const float h0 = y0 / std::sqrt((reach * reach) + (y0 * y0));
        const float h1 = y1 / std::sqrt((reach * reach) + (y1 * y1));
        const float hv = h0 + (u.y * (h1 - h0));
        const float squared = hv * hv;
        const float yv = squared < kBelowOne ? (hv * reach) / std::sqrt(1.0F - squared) : y1;
        return origin + (xAxis * xu) + (yAxis * std::fmax(y0, std::fmin(y1, yv))) + (zAxis * z0);
    }

 private:
    /** The angle between two unit vectors, without the loss acos has near zero and pi. **/
    MOYA_KERNEL static float angleBetween(const glm::vec3 & from, const glm::vec3 & to) {
        if (glm::dot(from, to) < 0.0F) {
            return kHalfTurn - (2.0F * std::asin(std::fmin(1.0F, glm::length(from + to) * 0.5F)));
        }
        return 2.0F * std::asin(std::fmin(1.0F, glm::length(to - from) * 0.5F));
    }
};

};  // namespace v3d::moya::kernel::light
