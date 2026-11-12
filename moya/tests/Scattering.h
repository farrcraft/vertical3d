/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {

/*
 * What the cases that check a BSDF's lobes share: whether its samples follow its density.
 */

/** A chi-squared fit: its statistic and its degrees of freedom. **/
struct Fit {
    float statistic { 0.0F };
    std::size_t freedom { 0 };

    /**
     * Whether the statistic lies below six deviations above its mean. The mean is the freedom
     * and the deviation the root of twice it, so a false failure comes less than once in a
     * billion.
     **/
    bool passes() const {
        const auto mean = static_cast<float>(freedom);
        return statistic < mean + (6.0F * std::sqrt(2.0F * mean));
    }
};

/**
 * How well the directions a BSDF samples for outgoing follow its density, by chi-squared. The
 * sphere about the shading normal is cut into bins even in the polar angle and the turn, so a
 * narrow lobe falls across many. Each bin expects the density integrated over its solid angle
 * on a finer grid. Bins expected to hold fewer than five samples are pooled into one. A specular
 * sample, or one the BSDF refused, falls in no bin, so the bins expect only what the density
 * holds.
 *
 * Each sample's density is checked against the BSDF's density for its direction.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count drawn, then the seed
inline Fit densityFit(const kernel::bsdf::Bsdf & bsdf, const glm::vec3 & outgoing, std::size_t drawn, std::uint32_t seed) {
    constexpr std::size_t kPolar = 40;
    constexpr std::size_t kTurns = 80;
    constexpr std::size_t kFiner = 12;
    constexpr float kPi = 3.14159265F;
    std::vector<double> counts(kPolar * kTurns, 0.0);
    for (std::size_t i = 0; i < drawn; ++i) {
        const glm::vec2 u { next(seed), next(seed) };
        const kernel::bsdf::Sample sample = bsdf.sample(outgoing, u, next(seed));
        if (!sample.found() || sample.specular) {
            continue;
        }
        BOOST_TEST_REQUIRE(sample.density == bsdf.density(outgoing, sample.incoming), boost::test_tools::tolerance(1.0e-3F));
        const glm::vec3 local = glm::normalize(bsdf.frame.toLocal(sample.incoming));
        const float polar = std::acos(std::fmin(std::fmax(local.z, -1.0F), 1.0F)) / kPi;
        const auto row = std::min(static_cast<std::size_t>(polar * kPolar), kPolar - 1);
        const float turn = (std::atan2(local.y, local.x) + kPi) / (2.0F * kPi);
        const auto around = std::min(static_cast<std::size_t>(turn * kTurns), kTurns - 1);
        counts[(row * kTurns) + around] += 1.0;
    }
    const double polarStep = kPi / (kPolar * kFiner);
    const double turnStep = 2.0 * kPi / (kTurns * kFiner);
    double pooledCount = 0.0;
    double pooledExpected = 0.0;
    Fit out;
    for (std::size_t row = 0; row < kPolar; ++row) {
        for (std::size_t around = 0; around < kTurns; ++around) {
            double integral = 0.0;
            for (std::size_t up = 0; up < kFiner; ++up) {
                const double polar = polarStep * (static_cast<double>((row * kFiner) + up) + 0.5);
                const auto z = static_cast<float>(std::cos(polar));
                const auto across = static_cast<float>(std::sin(polar));
                for (std::size_t over = 0; over < kFiner; ++over) {
                    const double angle = -kPi + (turnStep * (static_cast<double>((around * kFiner) + over) + 0.5));
                    const glm::vec3 local { across * static_cast<float>(std::cos(angle)), across * static_cast<float>(std::sin(angle)), z };
                    integral += static_cast<double>(bsdf.density(outgoing, bsdf.frame.toWorld(local))) * static_cast<double>(across) * polarStep * turnStep;
                }
            }
            const double expected = integral * static_cast<double>(drawn);
            const double counted = counts[(row * kTurns) + around];
            if (expected < 5.0) {
                pooledExpected += expected;
                pooledCount += counted;
                continue;
            }
            out.statistic += static_cast<float>((counted - expected) * (counted - expected) / expected);
            ++out.freedom;
        }
    }
    if (pooledExpected > 0.0) {
        out.statistic += static_cast<float>((pooledCount - pooledExpected) * (pooledCount - pooledExpected) / pooledExpected);
        ++out.freedom;
    } else {
        BOOST_TEST(pooledCount == 0.0);
    }
    if (out.freedom > 0) {
        --out.freedom;
    }
    return out;
}

/**
 * The mean weight of a BSDF's samples for outgoing, at the first wavelength: the share of light
 * from a white furnace it sends back along outgoing, as the integrator's scatter stage weighs each
 * sample. A sample that crosses the surface is multiplied by crossing, so a dielectric's
 * transmission can be counted by the energy it carries rather than its radiance.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count drawn, the seed, then the factor
inline float furnace(const kernel::bsdf::Bsdf & bsdf, const glm::vec3 & outgoing, std::size_t drawn, std::uint32_t seed, float crossing = 1.0F) {
    double sum = 0.0;
    for (std::size_t i = 0; i < drawn; ++i) {
        const glm::vec2 u { next(seed), next(seed) };
        const kernel::bsdf::Sample sample = bsdf.sample(outgoing, u, next(seed));
        if (!sample.found()) {
            continue;
        }
        const float cosine = sample.specular ? 1.0F : std::fabs(glm::dot(sample.incoming, bsdf.frame.normal));
        const bool crossed = glm::dot(sample.incoming, bsdf.geometric) * glm::dot(outgoing, bsdf.geometric) < 0.0F;
        sum += static_cast<double>(sample.value[0] * cosine * (crossed ? crossing : 1.0F) / sample.density);
    }
    return static_cast<float>(sum / static_cast<double>(drawn));
}

};  // namespace v3d::moya::tracing
