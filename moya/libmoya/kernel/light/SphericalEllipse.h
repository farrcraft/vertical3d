/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Elliptic.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/**
 * The directions from a point to a disk: an elliptic cone, sampled uniformly in solid angle by
 * Guillén and others' radial map. The cone's axis is axis. It meets the plane one unit along the
 * axis in an ellipse of semi-axes first along firstAxis and second along secondAxis. The solid
 * angle of the quarter of the cone from firstAxis towards secondAxis is quarter.
 *
 * Let A be first, B second, and tan psi = (A / B) tan t. At an azimuth t from firstAxis, the
 * quarter's solid angle so far is then
 * t - B / (A sqrt(1 + A^2)) Pi(1 - B^2 / A^2; psi | (A^2 - B^2) / (1 + A^2)).
 * It holds whichever semi-axis is the longer, since the integral's parameter may be below zero.
 * A direction is drawn by inverting that by Newton's method, and then its cosine to the axis
 * uniformly between the edge's and one.
 **/
struct SphericalEllipse {
    glm::vec3 axis { 0.0F, 0.0F, 1.0F };
    glm::vec3 firstAxis { 1.0F, 0.0F, 0.0F };
    glm::vec3 secondAxis { 0.0F, 1.0F, 0.0F };
    float first { 0.0F };
    float second { 0.0F };
    float quarter { 0.0F };

    /** The most Newton steps an inversion takes. **/
    static constexpr std::uint32_t kNewtonSteps = 16;

    /** How near its target an inversion's solid angle comes, as a share of the quarter. **/
    static constexpr float kTolerance = 1.0e-6F;

    /** Below this share of the distance squared off its axis, a disk is seen along its normal. **/
    static constexpr float kAlongNormal = 1.0e-12F;

    /** Below this share of a normal along x, x is crossed with it for a direction across it. **/
    static constexpr float kOffX = 0.9F;

    /** The quarters of the cone. **/
    static constexpr float kQuarters = 4.0F;

    /**
     * The cone from a point to a disk of a centre, a unit normal and a radius. The point lies off
     * the disk's plane. The cone's axis lies in the plane of the normal and the direction to the
     * centre, halfway between the directions to the two points of the rim in that plane. Its
     * first axis lies across that plane, and its second in it.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the disk
    MOYA_KERNEL static SphericalEllipse fromDisk(const glm::vec3 & from, const glm::vec3 & centre, const glm::vec3 & normal, float radius) {
        const glm::vec3 toCentre = centre - from;
        glm::vec3 across = glm::cross(normal, toCentre);
        if (!(glm::dot(across, across) > kAlongNormal * glm::dot(toCentre, toCentre))) {
            // seen along its normal, any direction in the disk's plane is a principal one
            across = glm::cross(normal, std::fabs(normal.x) < kOffX ? glm::vec3 { 1.0F, 0.0F, 0.0F } : glm::vec3 { 0.0F, 1.0F, 0.0F });
        }
        across = glm::normalize(across);
        const glm::vec3 along = glm::cross(across, normal);
        const glm::vec3 first = glm::normalize(toCentre + (along * radius));
        const glm::vec3 second = glm::normalize(toCentre - (along * radius));
        SphericalEllipse out;
        out.axis = glm::normalize(first + second);
        const glm::vec3 third = glm::cross(out.axis, across);
        const float halfCosine = glm::dot(first, out.axis);
        const float inPlane = std::sqrt(std::fmax(0.0F, 1.0F - (halfCosine * halfCosine))) / halfCosine;
        // the rim's point that lies in the plane of the axis and the direction across it
        const float rimCosine = -glm::dot(toCentre, third) / (radius * glm::dot(along, third));
        const float rimSine = std::sqrt(std::fmax(0.0F, 1.0F - (rimCosine * rimCosine)));
        const glm::vec3 rim = toCentre + (along * (radius * rimCosine)) + (across * (radius * rimSine));
        const float acrossPlane = std::fabs(glm::dot(rim, across) / glm::dot(rim, out.axis));
        out.firstAxis = across;
        out.secondAxis = third;
        out.first = acrossPlane;
        out.second = inPlane;
        out.quarter = out.swept(kQuarter);
        return out;
    }

    /** The cone's solid angle. **/
    MOYA_KERNEL float solidAngle() const {
        return kQuarters * quarter;
    }

    /**
     * A direction in the cone, drawn uniformly in solid angle from two random numbers: the first
     * picks the azimuth, and the second the cosine to the axis.
     **/
    MOYA_KERNEL glm::vec3 sample(const glm::vec2 & u) const {
        const float target = u.x * solidAngle();
        std::uint32_t turn = static_cast<std::uint32_t>(target / quarter);
        turn = turn > 3U ? 3U : turn;
        const float rest = target - (static_cast<float>(turn) * quarter);
        // an odd quarter runs back from its far edge, so the azimuth rises with u.x
        const bool backward = (turn & 1U) != 0U;
        const float within = invert(backward ? quarter - rest : rest);
        const float azimuth = backward ? (static_cast<float>(turn + 1U) * kQuarter) - within : (static_cast<float>(turn) * kQuarter) + within;
        const float cosine = 1.0F - (u.y * rise(azimuth));
        const float sine = std::sqrt(std::fmax(0.0F, 1.0F - (cosine * cosine)));
        return (firstAxis * (sine * std::cos(azimuth))) + (secondAxis * (sine * std::sin(azimuth))) + (axis * cosine);
    }

    /** One less the cosine to the axis of the cone's edge at an azimuth from its first axis. **/
    MOYA_KERNEL float rise(float azimuth) const {
        const float cosAzimuth = std::cos(azimuth);
        const float sinAzimuth = std::sin(azimuth);
        // the edge's sine squared, from 1 / sin^2 = cos^2 / sin^2(alpha) + sin^2 / sin^2(beta)
        const float firstSine = (first * first) / (1.0F + (first * first));
        const float secondSine = (second * second) / (1.0F + (second * second));
        const float squared = 1.0F / (((cosAzimuth * cosAzimuth) / firstSine) + ((sinAzimuth * sinAzimuth) / secondSine));
        return squared / (1.0F + std::sqrt(std::fmax(0.0F, 1.0F - squared)));
    }

    /** The solid angle of the cone between its first axis and an azimuth in [0, pi/2]. **/
    MOYA_KERNEL float swept(float azimuth) const {
        const float a2 = first * first;
        const float b2 = second * second;
        const float scale = second / (first * std::sqrt(1.0F + a2));
        const float psi = std::atan2(first * std::sin(azimuth), second * std::cos(azimuth));
        return azimuth - (scale * ellipticPi(1.0F - (b2 / a2), psi, (a2 - b2) / (1.0F + a2)));
    }

 private:
    static constexpr float kQuarter = 1.57079632679489661923F;

    /**
     * The azimuth in [0, pi/2] at which the swept solid angle reaches a share of the quarter, by
     * Newton's method held within a bracket that halves when a step leaves it.
     **/
    MOYA_KERNEL float invert(float target) const {
        float low = 0.0F;
        float high = kQuarter;
        float at = (target / quarter) * kQuarter;
        for (std::uint32_t step = 0; step < kNewtonSteps; ++step) {
            const float gap = swept(at) - target;
            if (std::fabs(gap) <= kTolerance * quarter) {
                break;
            }
            if (gap > 0.0F) {
                high = at;
            } else {
                low = at;
            }
            const float slope = rise(at);
            const float next = slope > 0.0F ? at - (gap / slope) : low - 1.0F;
            at = next > low && next < high ? next : 0.5F * (low + high);
        }
        return at;
    }
};

};  // namespace v3d::moya::kernel::light
