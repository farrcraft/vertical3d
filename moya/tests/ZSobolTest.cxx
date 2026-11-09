/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/sample/Hash.h>
#include <moya/libmoya/kernel/sample/Sobol.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>

#include <cmath>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>

namespace {

namespace sample = v3d::moya::kernel::sample;

/** Whether every grid of 2^n cells over the unit square holds exactly one of the 2^n points. **/
bool isZeroTwoNet(const std::vector<glm::vec2> & points) {
    const std::uint32_t log2 = sample::ZSobol::log2Ceiling(static_cast<std::uint32_t>(points.size()));
    for (std::uint32_t across = 0; across <= log2; ++across) {
        const float columns = static_cast<float>(1U << across);
        const float rows = static_cast<float>(1U << (log2 - across));
        std::set<std::pair<int, int>> cells;
        for (const glm::vec2 & point : points) {
            cells.emplace(static_cast<int>(point.x * columns), static_cast<int>(point.y * rows));
        }
        if (cells.size() != points.size()) {
            return false;
        }
    }
    return true;
}

/** One for a point inside the quarter of the unit disc in the unit square, and zero outside it. **/
float inDisc(const glm::vec2 & point) {
    return (point.x * point.x) + (point.y * point.y) < 1.0F ? 1.0F : 0.0F;
}

/** The n-th of a run of independent uniform numbers in [0, 1). **/
float independent(std::uint64_t n) {
    return sample::toUnit(static_cast<std::uint32_t>(sample::mix(n + 1U) >> 32U));
}

// the sequence's first eight values in each dimension, unscrambled, in index order rather than
// the Gray-code order the sequence is often listed in
BOOST_AUTO_TEST_CASE(moya_zsobol_the_matrices_give_the_sobol_sequence_test) {
    const float first[8] = { 0.0F, 0.5F, 0.25F, 0.75F, 0.125F, 0.625F, 0.375F, 0.875F };
    const float second[8] = { 0.0F, 0.5F, 0.75F, 0.25F, 0.625F, 0.125F, 0.375F, 0.875F };
    for (std::uint64_t index = 0; index < 8; ++index) {
        BOOST_TEST(sample::toUnit(sample::sobol(index, 0)) == first[index]);
        BOOST_TEST(sample::toUnit(sample::sobol(index, 1)) == second[index]);
    }
}

BOOST_AUTO_TEST_CASE(moya_zsobol_a_sample_depends_only_on_its_arguments_test) {
    const sample::ZSobol one(64, { 640, 480 }, 7, 3);
    const sample::ZSobol two(64, { 640, 480 }, 7, 3);
    for (std::uint32_t dimension = 0; dimension < 8; ++dimension) {
        for (std::uint32_t index = 0; index < 64; ++index) {
            const glm::uvec2 pixel { 311, 207 };
            const float value = one.get1D(pixel, index, dimension);
            BOOST_TEST(value >= 0.0F);
            BOOST_TEST(value < 1.0F);
            BOOST_TEST(value == one.get1D(pixel, index, dimension));
            BOOST_TEST(value == two.get1D(pixel, index, dimension));
            BOOST_TEST((one.get2D(pixel, index, dimension) == two.get2D(pixel, index, dimension)));
        }
    }
}

// a pixel takes an aligned run of the sequence, which scrambling and reordering keep a net, for an
// odd power of two as for an even one
BOOST_AUTO_TEST_CASE(moya_zsobol_a_pixels_samples_are_a_zero_two_net_test) {
    const glm::uvec2 pixels[3] = { { 0, 0 }, { 5, 3 }, { 17, 30 } };
    for (std::uint32_t log2 = 0; log2 <= 8; ++log2) {
        const sample::ZSobol sampler(1U << log2, { 32, 32 }, 11, 0);
        for (const glm::uvec2 & pixel : pixels) {
            for (const std::uint32_t dimension : { 0U, 3U, 10U }) {
                std::vector<glm::vec2> points;
                points.reserve(sampler.samplesPerPixel());
                for (std::uint32_t index = 0; index < sampler.samplesPerPixel(); ++index) {
                    points.push_back(sampler.get2D(pixel, index, dimension));
                }
                BOOST_TEST(isZeroTwoNet(points), "2^" << log2 << " samples in dimension " << dimension);
            }
        }
    }
}

// the area of a quarter disc, estimated in each pixel of an image from 256 samples: the error over
// the image is well below that of independent uniform numbers
BOOST_AUTO_TEST_CASE(moya_zsobol_converges_faster_than_independent_samples_test) {
    constexpr std::uint32_t kSide = 32;
    constexpr std::uint32_t kSamples = 256;
    const float exact = std::acos(-1.0F) / 4.0F;
    const sample::ZSobol sampler(kSamples, { kSide, kSide }, 1, 0);
    std::uint64_t drawn = 0;

    double sobolError = 0.0;
    double randomError = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            float sobolSum = 0.0F;
            float randomSum = 0.0F;
            for (std::uint32_t index = 0; index < kSamples; ++index) {
                sobolSum += inDisc(sampler.get2D({ x, y }, index, 2));
                randomSum += inDisc({ independent(drawn++), independent(drawn++) });
            }
            const double sobolMiss = (sobolSum / kSamples) - exact;
            const double randomMiss = (randomSum / kSamples) - exact;
            sobolError += sobolMiss * sobolMiss;
            randomError += randomMiss * randomMiss;
        }
    }
    BOOST_TEST(sobolError < randomError / 4.0);
}

BOOST_AUTO_TEST_CASE(moya_zsobol_each_frame_takes_its_own_samples_test) {
    const sample::ZSobol first(16, { 64, 64 }, 5, 0);
    const sample::ZSobol second(16, { 64, 64 }, 5, 1);
    int same = 0;
    for (std::uint32_t y = 0; y < 4; ++y) {
        for (std::uint32_t x = 0; x < 4; ++x) {
            for (std::uint32_t index = 0; index < 16; ++index) {
                same += first.get1D({ x, y }, index, 4) == second.get1D({ x, y }, index, 4) ? 1 : 0;
            }
        }
    }
    BOOST_TEST(same == 0);
}

};  // namespace
