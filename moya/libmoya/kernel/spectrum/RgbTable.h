/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/RgbSigmoid.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::spectrum {

/**
 * Jakob and Hanika's table, which gives the RgbSigmoid of an ACEScg colour in [0, 1]. It is fitted
 * at build time, and lives in device memory; this is a view of it.
 *
 * It holds three cubes, one for each channel that can be a colour's largest. A cube's axes are the
 * largest channel's value, on the scale its nodes give, and the next two channels, in order,
 * divided by the largest. Each of a cube's resolution^3 points holds c0, c1 and c2.
 **/
struct RgbTable {
    /** The points along each axis of a cube. **/
    std::uint32_t resolution { 0 };
    /** The largest channel's value at each point along its axis, rising from zero to one. **/
    Array<const float> nodes;
    /** The coefficients: by cube, then by the largest channel, the third and the second. **/
    Array<const float> coefficients;

    /** The coefficients for an ACEScg colour, which is clamped to [0, 1] first. **/
    MOYA_KERNEL RgbSigmoid operator()(const glm::vec3 & colour) const {
        const float rgb[3] = { clamp(colour.x), clamp(colour.y), clamp(colour.z) };
        // a grey's spectrum is constant, and its sigmoid is a constant that gives it
        if (rgb[0] == rgb[1] && rgb[1] == rgb[2]) {
            return { 0.0F, 0.0F, constant(rgb[0]) };
        }
        int largest = 0;
        for (int channel = 1; channel < 3; ++channel) {
            largest = rgb[channel] > rgb[largest] ? channel : largest;
        }
        const float z = rgb[largest];
        const auto last = static_cast<float>(resolution - 1);
        const float x = rgb[(largest + 1) % 3] / z * last;
        const float y = rgb[(largest + 2) % 3] / z * last;
        const std::uint32_t xi = cell(x);
        const std::uint32_t yi = cell(y);
        std::uint32_t zi = 0;
        // the nodes rise, so the cell holding z is the last node at or below it
        std::uint32_t high = resolution - 1;
        while (high - zi > 1) {
            const std::uint32_t middle = (zi + high) / 2;
            if (nodes[middle] <= z) {
                zi = middle;
            } else {
                high = middle;
            }
        }
        const float dx = x - static_cast<float>(xi);
        const float dy = y - static_cast<float>(yi);
        const float dz = (z - nodes[zi]) / (nodes[zi + 1] - nodes[zi]);
        float out[3];
        for (int k = 0; k < 3; ++k) {
            const auto at = [&](std::uint32_t ox, std::uint32_t oy, std::uint32_t oz) {
                std::uint32_t point = (static_cast<std::uint32_t>(largest) * resolution) + zi + oz;
                point = (point * resolution) + yi + oy;
                point = (point * resolution) + xi + ox;
                return coefficients[(point * 3U) + static_cast<std::uint32_t>(k)];
            };
            const auto lerp = [](float a0, float a1, float t) { return a0 + (t * (a1 - a0)); };
            const float near = lerp(lerp(at(0, 0, 0), at(1, 0, 0), dx), lerp(at(0, 1, 0), at(1, 1, 0), dx), dy);
            const float far = lerp(lerp(at(0, 0, 1), at(1, 0, 1), dx), lerp(at(0, 1, 1), at(1, 1, 1), dx), dy);
            out[k] = lerp(near, far, dz);
        }
        return { out[0], out[1], out[2] };
    }

    /** A channel clamped to [0, 1], and zero for a NaN. **/
    MOYA_KERNEL static float clamp(float value) {
        if (!(value > 0.0F)) {
            return 0.0F;
        }
        return value < 1.0F ? value : 1.0F;
    }

 private:
    /** The constant c2 at which sigmoid gives a grey's value, infinite at zero and one. **/
    MOYA_KERNEL static float constant(float grey) {
        if (grey <= 0.0F) {
            return -std::numeric_limits<float>::infinity();
        }
        if (grey >= 1.0F) {
            return std::numeric_limits<float>::infinity();
        }
        return (grey - 0.5F) / std::sqrt(grey * (1.0F - grey));
    }

    /** The cell of a coordinate in [0, resolution - 1], whose upper neighbour exists. **/
    MOYA_KERNEL std::uint32_t cell(float coordinate) const {
        const auto index = static_cast<std::uint32_t>(coordinate);
        return index < resolution - 2 ? index : resolution - 2;
    }
};

};  // namespace v3d::moya::kernel::spectrum
