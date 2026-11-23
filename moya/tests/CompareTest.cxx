/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>

namespace {

namespace kernel = v3d::moya::kernel;
namespace film = v3d::moya::film;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** The side of each test picture, in pixels. **/
constexpr std::uint32_t kSide = 32;

/** The samples each pixel of uniform() takes. **/
constexpr std::uint32_t kSamples = 64;

/** Each of weighted()'s samples: its weight and its radiance. **/
constexpr int kWeighted = 4;
constexpr float kWeights[kWeighted] = { 1.0F, 2.0F, 0.5F, -0.25F };
constexpr float kRadiances[kWeighted] = { 1.0F, 3.0F, 0.25F, 2.0F };

/** What uniform() and weighted() draw from. **/
struct Params {
    kernel::film::Pixels pixels;
    kernel::sample::ZSobol sampler;
    std::uint32_t samples { kSamples };

    /** The radiance each sample's random number is scaled by. **/
    float scale { 2.0F };
};

/** The wavelengths every sample here is drawn at, so a sample's luminance is its radiance's. **/
kernel::spectrum::SampledWavelengths fixedWavelengths() {
    return kernel::spectrum::SampledWavelengths::visible(0.5F);
}

/** The luminance of a radiance of one at the fixed wavelengths. **/
float unitLuminance() {
    return kernel::spectrum::toXyz(kernel::spectrum::SampledSpectrum(1.0F), fixedWavelengths()).y;
}

/** Adds samples of weight one whose radiance is uniform over zero to the scale. **/
void uniform(const Params & params, kernel::Item item) {
    const glm::uvec2 pixel { item.x, item.y };
    for (std::uint32_t index = 0; index < params.samples; ++index) {
        const float radiance = params.sampler.get1D(pixel, index, 0) * params.scale;
        params.pixels.add(item, 1.0F, kernel::spectrum::SampledSpectrum(radiance), fixedWavelengths());
    }
}

/** Adds weighted()'s samples to each pixel. **/
void weighted(const Params & params, kernel::Item item) {
    for (int sample = 0; sample < kWeighted; ++sample) {
        params.pixels.add(item, kWeights[sample], kernel::spectrum::SampledSpectrum(kRadiances[sample]), fixedWavelengths());
    }
}

/** A picture of the given seed, from one pass of a function. **/
film::Snapshot picture(kernel::Function<Params> function, std::uint32_t seed, float scale, std::uint32_t samples) {
    CpuDevice device;
    film::Film film(device, { kSide, kSide }, {});
    film.pass([&](const kernel::film::Pixels & pixels) {
        const Params params { .pixels = pixels, .sampler = kernel::sample::ZSobol(samples, { kSide, kSide }, seed, 0), .samples = samples,
            .scale = scale };
        device.launch(kSide, kSide, params, function);
    });
    return film.snapshot();
}

/** A picture of uniform()'s samples. **/
film::Snapshot uniformPicture(std::uint32_t seed, float scale = 2.0F, std::uint32_t samples = kSamples) {
    return picture(&uniform, seed, scale, samples);
}

/** The standard error of kSamples uniform values from zero to two, times unit luminance. **/
float theoreticalError() {
    return unitLuminance() * std::sqrt((4.0F / 12.0F) / static_cast<float>(kSamples));
}

// a pixel of equal weights has the error of a mean: the samples' variance over their count
BOOST_AUTO_TEST_CASE(moya_compare_a_pixels_error_is_the_standard_error_of_its_mean_test) {
    const film::Snapshot snapshot = uniformPicture(3);
    const kernel::sample::ZSobol sampler(kSamples, { kSide, kSide }, 3, 0);
    for (const glm::uvec2 pixel : { glm::uvec2 { 0, 0 }, glm::uvec2 { 17, 5 }, glm::uvec2 { 31, 31 } }) {
        double sum = 0.0;
        double squares = 0.0;
        for (std::uint32_t index = 0; index < kSamples; ++index) {
            const double value = static_cast<double>(sampler.get1D(pixel, index, 0)) * 2.0 * unitLuminance();
            sum += value;
            squares += value * value;
        }
        const double mean = sum / kSamples;
        const double variance = (squares - (kSamples * mean * mean)) / (kSamples - 1);
        BOOST_TEST_CONTEXT("pixel " << pixel.x << ", " << pixel.y) {
            BOOST_TEST(snapshot.error(pixel.x, pixel.y) == std::sqrt(variance / kSamples), boost::test_tools::tolerance(0.001));
            BOOST_TEST(snapshot.error(pixel.x, pixel.y) == theoreticalError(), boost::test_tools::tolerance(0.15F));
        }
    }
}

// samples of unequal weights, one of them negative, have the error of a weighted mean. Its
// variance is each squared weight times its squared distance from the mean, summed, over the
// squared sum of the weights. It is scaled by the effective count over one less
BOOST_AUTO_TEST_CASE(moya_compare_weighted_samples_have_the_error_of_a_weighted_mean_test) {
    const film::Snapshot snapshot = picture(&weighted, 0, 1.0F, 1);
    double total = 0.0;
    double squaredWeights = 0.0;
    double weightedSum = 0.0;
    for (int sample = 0; sample < kWeighted; ++sample) {
        total += kWeights[sample];
        squaredWeights += static_cast<double>(kWeights[sample]) * kWeights[sample];
        weightedSum += static_cast<double>(kWeights[sample]) * kRadiances[sample] * unitLuminance();
    }
    const double mean = weightedSum / total;
    double spread = 0.0;
    for (int sample = 0; sample < kWeighted; ++sample) {
        const double distance = (static_cast<double>(kRadiances[sample]) * unitLuminance()) - mean;
        spread += static_cast<double>(kWeights[sample]) * kWeights[sample] * distance * distance;
    }
    const double count = (total * total) / squaredWeights;
    const double expected = std::sqrt((spread / (total * total)) * (count / (count - 1.0)));
    BOOST_TEST(film::luminance(snapshot, 4, 9).value == mean, boost::test_tools::tolerance(0.0001));
    BOOST_TEST(snapshot.error(4, 9) == expected, boost::test_tools::tolerance(0.001));
}

BOOST_AUTO_TEST_CASE(moya_compare_a_picture_passes_against_its_mean_test) {
    const film::Snapshot snapshot = uniformPicture(5);
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return unitLuminance(); });
    BOOST_TEST(furthest.sigmas < 4.0F);
    BOOST_TEST(furthest.unmeasured == 0U);
    BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), unitLuminance()) < 3.0F);
}

// the expected value moved by four of the mean's true standard errors lies about four of the
// estimated errors away, at every pixel and over the picture
BOOST_AUTO_TEST_CASE(moya_compare_a_picture_moved_by_four_errors_fails_test) {
    const film::Snapshot snapshot = uniformPicture(5);
    const float moved = unitLuminance() + (4.0F * theoreticalError());
    int near = 0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            near += film::sigmas(film::luminance(snapshot, x, y), moved) > 3.0F ? 0 : 1;
        }
    }
    BOOST_TEST(near == 0);
    const float regionMoved = unitLuminance() + (4.0F * theoreticalError() / static_cast<float>(kSide));
    BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), regionMoved) > 3.0F);
}

BOOST_AUTO_TEST_CASE(moya_compare_two_pictures_of_one_scene_pass_against_each_other_test) {
    const film::Snapshot first = uniformPicture(1);
    const film::Snapshot second = uniformPicture(2);
    const film::Furthest furthest = film::furthest(first, second);
    BOOST_TEST(furthest.sigmas < 4.0F);
    BOOST_TEST(furthest.unmeasured == 0U);
    BOOST_TEST(film::sigmas(film::mean(first, { 0, 0 }, { kSide, kSide }), film::mean(second, { 0, 0 }, { kSide, kSide })) < 3.0F);
}

// a tenth brighter is within the noise of a pixel, and far outside the noise of the picture
BOOST_AUTO_TEST_CASE(moya_compare_two_pictures_of_different_light_fail_over_the_picture_test) {
    const film::Snapshot dim = uniformPicture(1);
    const film::Snapshot bright = uniformPicture(2, 2.2F);
    BOOST_TEST(film::sigmas(film::mean(dim, { 0, 0 }, { kSide, kSide }), film::mean(bright, { 0, 0 }, { kSide, kSide })) > 10.0F);
}

BOOST_AUTO_TEST_CASE(moya_compare_a_pixel_of_one_sample_is_not_measured_test) {
    const film::Snapshot snapshot = uniformPicture(1, 2.0F, 1);
    BOOST_TEST(std::isinf(snapshot.error(3, 3)));
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 100.0F; });
    BOOST_TEST(furthest.sigmas == 0.0F);
    BOOST_TEST(furthest.unmeasured == kSide * kSide);
}

// an estimate of no noise is measured against a ten-thousandth of its value. The rounding of a
// render whose samples all agree is then never a difference, and a part in a thousand still is
BOOST_AUTO_TEST_CASE(moya_compare_an_estimate_of_no_error_is_measured_by_rounding_test) {
    BOOST_TEST(film::sigmas(film::Estimate { .value = 1.00003F, .error = 0.0F }, 1.0F) < 1.0F);
    BOOST_TEST(film::sigmas(film::Estimate { .value = 4.0001F, .error = 0.0F }, 4.0F) < 1.0F);
    BOOST_TEST(film::sigmas(film::Estimate { .value = 1.001F, .error = 0.0F }, 1.0F) > 5.0F);
    BOOST_TEST(film::sigmas(film::Estimate { .value = 0.0F, .error = 0.0F }, film::Estimate { .value = 0.00003F, .error = 0.0F }) < 1.0F);
}

BOOST_AUTO_TEST_CASE(moya_compare_a_pixel_that_is_not_a_number_is_furthest_test) {
    const film::Snapshot snapshot = uniformPicture(1);
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 & pixel) {
        return pixel == glm::uvec2 { 7, 2 } ? std::numeric_limits<float>::quiet_NaN() : unitLuminance();
    });
    BOOST_TEST(std::isnan(furthest.sigmas));
    BOOST_TEST(furthest.pixel.x == 7U);
    BOOST_TEST(furthest.pixel.y == 2U);
}

BOOST_AUTO_TEST_CASE(moya_compare_refuses_what_it_cannot_compare_test) {
    const film::Snapshot snapshot = uniformPicture(1);
    BOOST_CHECK_THROW(film::mean(snapshot, { 4, 4 }, { 4, 8 }), std::invalid_argument);
    BOOST_CHECK_THROW(film::mean(snapshot, { 0, 0 }, { kSide + 1, kSide }), std::invalid_argument);

    CpuDevice device;
    const film::Film other(device, { kSide, kSide + 1 }, {});
    BOOST_CHECK_THROW(film::furthest(snapshot, other.snapshot()), std::invalid_argument);
}

};  // namespace
