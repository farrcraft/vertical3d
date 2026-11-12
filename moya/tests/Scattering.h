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
 * sphere about the shading normal is cut into bins even in the cosine and the turn, so they are
 * of equal area. Each bin expects the density integrated over it on a finer grid. Bins expected
 * to hold fewer than five samples are pooled into one. A specular sample, or one the BSDF
 * refused, falls in no bin, so the bins expect only what the density holds.
 *
 * Each sample's density is checked against the BSDF's density for its direction.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count drawn, then the seed
inline Fit densityFit(const kernel::bsdf::Bsdf & bsdf, const glm::vec3 & outgoing, std::size_t drawn, std::uint32_t seed) {
    constexpr std::size_t kHeights = 20;
    constexpr std::size_t kTurns = 20;
    constexpr std::size_t kFiner = 8;
    constexpr float kPi = 3.14159265F;
    std::vector<double> counts(kHeights * kTurns, 0.0);
    for (std::size_t i = 0; i < drawn; ++i) {
        const glm::vec2 u { next(seed), next(seed) };
        const kernel::bsdf::Sample sample = bsdf.sample(outgoing, u);
        if (!sample.found() || sample.specular) {
            continue;
        }
        BOOST_TEST_REQUIRE(sample.density == bsdf.density(outgoing, sample.incoming), boost::test_tools::tolerance(1.0e-4F));
        const glm::vec3 local = bsdf.frame.toLocal(sample.incoming);
        const auto height = std::min(static_cast<std::size_t>((local.z + 1.0F) * 0.5F * kHeights), kHeights - 1);
        const float turn = (std::atan2(local.y, local.x) + kPi) / (2.0F * kPi);
        const auto around = std::min(static_cast<std::size_t>(turn * kTurns), kTurns - 1);
        counts[(height * kTurns) + around] += 1.0;
    }
    const double cell = (2.0 / (kHeights * kFiner)) * (2.0 * kPi / (kTurns * kFiner));
    double pooledCount = 0.0;
    double pooledExpected = 0.0;
    Fit out;
    for (std::size_t height = 0; height < kHeights; ++height) {
        for (std::size_t around = 0; around < kTurns; ++around) {
            double integral = 0.0;
            for (std::size_t up = 0; up < kFiner; ++up) {
                for (std::size_t over = 0; over < kFiner; ++over) {
                    const float z = -1.0F + (2.0F * ((static_cast<float>((height * kFiner) + up) + 0.5F) / (kHeights * kFiner)));
                    const float angle = -kPi + (2.0F * kPi * ((static_cast<float>((around * kFiner) + over) + 0.5F) / (kTurns * kFiner)));
                    const float across = std::sqrt(std::fmax(0.0F, 1.0F - (z * z)));
                    const glm::vec3 local { across * std::cos(angle), across * std::sin(angle), z };
                    integral += static_cast<double>(bsdf.density(outgoing, bsdf.frame.toWorld(local))) * cell;
                }
            }
            const double expected = integral * static_cast<double>(drawn);
            const double counted = counts[(height * kTurns) + around];
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

};  // namespace v3d::moya::tracing
