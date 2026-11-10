/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Compare.h>

#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::film {

namespace {

/** The least error a comparison divides by, for an expected value. **/
float floorFor(float expected) {
    constexpr float kRelative = 1.0e-6F;
    return kRelative * std::max(1.0F, std::abs(expected));
}

/** How far a difference lies, by an error, as sigmas() measures it. **/
float measure(float difference, float error, float expected) {
    if (std::isinf(error)) {
        return 0.0F;
    }
    return std::abs(difference) / std::max(error, floorFor(expected));
}

/** Records a pixel's distance in a running furthest. **/
void record(Furthest & out, const glm::uvec2 & pixel, float distance, bool measured) {
    if (!measured) {
        ++out.unmeasured;
        return;
    }
    // a NaN is as far as anything can be, so the first one found is kept
    if (std::isnan(out.sigmas)) {
        return;
    }
    if (std::isnan(distance) || distance > out.sigmas) {
        out.pixel = pixel;
        out.sigmas = distance;
    }
}

};  // namespace

Estimate luminance(const Snapshot & snapshot, std::uint32_t x, std::uint32_t y) {
    const glm::vec3 rgb { snapshot.at(x, y, 0), snapshot.at(x, y, 1), snapshot.at(x, y, 2) };
    return Estimate { .value = kernel::spectrum::acescgToXyz(rgb).y, .error = snapshot.error(x, y) };
}

Estimate mean(const Snapshot & snapshot, const glm::uvec2 & from, const glm::uvec2 & to) {
    if (to.x <= from.x || to.y <= from.y || to.x > snapshot.resolution.x || to.y > snapshot.resolution.y) {
        throw std::invalid_argument("a region of a snapshot that holds no pixel or reaches outside it");
    }
    double sum = 0.0;
    double squares = 0.0;
    for (std::uint32_t y = from.y; y < to.y; ++y) {
        for (std::uint32_t x = from.x; x < to.x; ++x) {
            const Estimate pixel = luminance(snapshot, x, y);
            sum += pixel.value;
            squares += static_cast<double>(pixel.error) * pixel.error;
        }
    }
    const double count = static_cast<double>(to.x - from.x) * (to.y - from.y);
    return Estimate { .value = static_cast<float>(sum / count), .error = static_cast<float>(std::sqrt(squares) / count) };
}

float sigmas(const Estimate & estimate, float expected) {
    return measure(estimate.value - expected, estimate.error, expected);
}

float sigmas(const Estimate & a, const Estimate & b) {
    const float error = std::sqrt((a.error * a.error) + (b.error * b.error));
    return measure(a.value - b.value, error, std::max(std::abs(a.value), std::abs(b.value)));
}

Furthest furthest(const Snapshot & snapshot, const std::function<float(const glm::uvec2 &)> & expected) {
    Furthest out;
    for (std::uint32_t y = 0; y < snapshot.resolution.y; ++y) {
        for (std::uint32_t x = 0; x < snapshot.resolution.x; ++x) {
            const Estimate pixel = luminance(snapshot, x, y);
            record(out, { x, y }, sigmas(pixel, expected({ x, y })), !std::isinf(pixel.error));
        }
    }
    return out;
}

Furthest furthest(const Snapshot & a, const Snapshot & b) {
    if (a.resolution != b.resolution) {
        throw std::invalid_argument("two snapshots of different resolutions");
    }
    Furthest out;
    for (std::uint32_t y = 0; y < a.resolution.y; ++y) {
        for (std::uint32_t x = 0; x < a.resolution.x; ++x) {
            const Estimate first = luminance(a, x, y);
            const Estimate second = luminance(b, x, y);
            record(out, { x, y }, sigmas(first, second), !std::isinf(first.error) && !std::isinf(second.error));
        }
    }
    return out;
}

};  // namespace v3d::moya::film
