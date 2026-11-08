/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/spectrum/CieObserver.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Visible.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>
#include <functional>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace spectrum = v3d::moya::kernel::spectrum;

/** A spectrum's XYZ, summed over the observer's range at 1 nm steps and divided by kYIntegral. **/
glm::vec3 exact(const std::function<float(float)> & power) {
    glm::vec3 sum(0.0F);
    for (int lambda = 360; lambda <= 830; ++lambda) {
        sum += spectrum::observer(static_cast<float>(lambda)) * power(static_cast<float>(lambda));
    }
    return sum / spectrum::kYIntegral;
}

/** The chromaticity x and y of an XYZ. **/
glm::vec2 chromaticity(const glm::vec3 & xyz) {
    const float total = xyz.x + xyz.y + xyz.z;
    return { xyz.x / total, xyz.y / total };
}

BOOST_AUTO_TEST_CASE(moya_spectrum_d65_has_its_published_white_point_test) {
    const glm::vec2 white = chromaticity(exact([](float lambda) { return spectrum::d65(lambda); }));
    BOOST_TEST(white.x == 0.31272F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(white.y == 0.32903F, boost::test_tools::tolerance(0.0001F));
}

BOOST_AUTO_TEST_CASE(moya_spectrum_illuminant_a_has_its_published_white_point_test) {
    BOOST_TEST(spectrum::illuminantA(560.0F) == 100.0F, boost::test_tools::tolerance(0.0001F));
    const glm::vec2 white = chromaticity(exact([](float lambda) { return spectrum::illuminantA(lambda); }));
    BOOST_TEST(white.x == 0.44758F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(white.y == 0.40745F, boost::test_tools::tolerance(0.0001F));
}

// the observer's three functions have nearly equal integrals, so an equal-energy spectrum lies at
// the centre of the chromaticity diagram, and a constant spectrum of one has a luminance of one
BOOST_AUTO_TEST_CASE(moya_spectrum_a_constant_spectrum_is_equal_energy_white_test) {
    const glm::vec3 xyz = exact([](float) { return 1.0F; });
    BOOST_TEST(xyz.y == 1.0F, boost::test_tools::tolerance(0.00001F));
    const glm::vec2 white = chromaticity(xyz);
    BOOST_TEST(white.x == 1.0F / 3.0F, boost::test_tools::tolerance(0.0005F));
    BOOST_TEST(white.y == 1.0F / 3.0F, boost::test_tools::tolerance(0.0005F));
}

BOOST_AUTO_TEST_CASE(moya_spectrum_the_y_integral_is_the_tables_test) {
    double sum = 0.0;
    for (const auto & row : spectrum::kObserver) {
        sum += row[1];
    }
    BOOST_TEST(static_cast<float>(sum) == spectrum::kYIntegral, boost::test_tools::tolerance(0.00001F));
}

BOOST_AUTO_TEST_CASE(moya_spectrum_the_observer_interpolates_between_rows_and_ends_at_its_range_test) {
    BOOST_TEST(spectrum::observer(555.0F).y == spectrum::kObserver[195][1]);
    const float between = (spectrum::kObserver[195][0] + spectrum::kObserver[196][0]) / 2.0F;
    BOOST_TEST(spectrum::observer(555.5F).x == between, boost::test_tools::tolerance(0.00001F));
    BOOST_TEST(spectrum::observer(830.0F).y == spectrum::kObserver[470][1]);
    BOOST_TEST(spectrum::observer(359.0F).y == 0.0F);
    BOOST_TEST(spectrum::observer(831.0F).y == 0.0F);
    BOOST_TEST(spectrum::observer(std::nanf("")).y == 0.0F);
}

BOOST_AUTO_TEST_CASE(moya_spectrum_the_visible_density_integrates_to_one_test) {
    double sum = 0.0;
    for (int lambda = 360; lambda < 830; ++lambda) {
        sum += (spectrum::visiblePdf(static_cast<float>(lambda)) + spectrum::visiblePdf(static_cast<float>(lambda + 1))) / 2.0;
    }
    BOOST_TEST(sum == 1.0, boost::test_tools::tolerance(0.001));
    BOOST_TEST(spectrum::visiblePdf(359.0F) == 0.0F);
    BOOST_TEST(spectrum::visiblePdf(831.0F) == 0.0F);
}

// a sampler draws from its density when its slope is one over the density at every point
BOOST_AUTO_TEST_CASE(moya_spectrum_wavelengths_are_drawn_from_the_visible_density_test) {
    BOOST_TEST(spectrum::sampleVisible(0.0F) == 360.0F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(spectrum::sampleVisible(1.0F) == 830.0F, boost::test_tools::tolerance(0.0001F));
    const float step = 0.001F;
    for (const float u : { 0.05F, 0.25F, 0.5F, 0.75F, 0.95F }) {
        const float slope = (spectrum::sampleVisible(u + step) - spectrum::sampleVisible(u - step)) / (2.0F * step);
        BOOST_TEST_CONTEXT("u " << u) {
            BOOST_TEST(slope == 1.0F / spectrum::visiblePdf(spectrum::sampleVisible(u)), boost::test_tools::tolerance(0.01F));
        }
    }
}

// an RGB build carries no wavelengths, so the next three cases hold only in a spectral one
#ifndef MOYA_RGB
BOOST_AUTO_TEST_CASE(moya_spectrum_a_path_carries_four_wavelengths_spread_through_the_distribution_test) {
    const spectrum::SampledWavelengths wavelengths = spectrum::SampledWavelengths::visible(0.6F);
    BOOST_TEST(wavelengths.lambda[0] == spectrum::sampleVisible(0.6F));
    BOOST_TEST(wavelengths.lambda[1] == spectrum::sampleVisible(0.85F), boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(wavelengths.lambda[2] == spectrum::sampleVisible(0.1F), boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(wavelengths.lambda[3] == spectrum::sampleVisible(0.35F), boost::test_tools::tolerance(0.0001F));
    for (int i = 0; i < spectrum::kSamples; ++i) {
        BOOST_TEST(wavelengths.pdf[i] == spectrum::visiblePdf(wavelengths.lambda[i]));
        BOOST_TEST(wavelengths.density()[i] == wavelengths.pdf[i]);
    }
}

// hero sampling's estimate of D65 converges to the integral of its table
BOOST_AUTO_TEST_CASE(moya_spectrum_hero_sampling_converges_to_a_spectrums_xyz_test) {
    const int count = 4096;
    glm::vec3 sum(0.0F);
    for (int k = 0; k < count; ++k) {
        const float u = (static_cast<float>(k) + 0.5F) / static_cast<float>(count);
        const spectrum::SampledWavelengths wavelengths = spectrum::SampledWavelengths::visible(u);
        sum += spectrum::toXyz(spectrum::d65(wavelengths), wavelengths);
    }
    const glm::vec3 estimate = sum / static_cast<float>(count);
    const glm::vec3 truth = exact([](float lambda) { return spectrum::d65(lambda); });
    BOOST_TEST(estimate.x == truth.x, boost::test_tools::tolerance(0.002F));
    BOOST_TEST(estimate.y == truth.y, boost::test_tools::tolerance(0.002F));
    BOOST_TEST(estimate.z == truth.z, boost::test_tools::tolerance(0.002F));
}

BOOST_AUTO_TEST_CASE(moya_spectrum_a_wavelength_drawn_with_no_density_adds_nothing_test) {
    spectrum::SampledWavelengths wavelengths = spectrum::SampledWavelengths::visible(0.3F);
    const glm::vec3 all = spectrum::toXyz(spectrum::SampledSpectrum(1.0F), wavelengths);
    wavelengths.pdf[2] = 0.0F;
    const glm::vec3 three = spectrum::toXyz(spectrum::SampledSpectrum(1.0F), wavelengths);
    BOOST_TEST(three.y < all.y);
    BOOST_TEST(std::isfinite(three.y));
}
#endif

// written for any number of values, so it holds in an RGB build as in a spectral one
BOOST_AUTO_TEST_CASE(moya_spectrum_arithmetic_is_per_wavelength_test) {
    const int last = spectrum::kSamples - 1;
    spectrum::SampledSpectrum ramp;
    for (int i = 0; i < spectrum::kSamples; ++i) {
        ramp[i] = static_cast<float>(i + 1);
    }
    const spectrum::SampledSpectrum twos(2.0F);
    BOOST_TEST((ramp * twos)[last] == 2.0F * static_cast<float>(last + 1));
    BOOST_TEST((ramp + twos)[0] == 3.0F);
    BOOST_TEST((ramp - twos)[1] == 0.0F);
    BOOST_TEST((ramp / 2.0F)[2] == 1.5F);
    BOOST_TEST((0.5F * ramp)[last] == 0.5F * static_cast<float>(last + 1));
    BOOST_TEST(ramp.average() == static_cast<float>(spectrum::kSamples + 1) / 2.0F);
    BOOST_TEST(ramp.maximum() == static_cast<float>(spectrum::kSamples));
    BOOST_TEST(!ramp.black());
    BOOST_TEST(spectrum::SampledSpectrum().black());
    spectrum::SampledSpectrum zero(2.0F);
    zero[1] = 0.0F;
    BOOST_TEST(spectrum::safeDivide(ramp, zero)[1] == 0.0F);
    BOOST_TEST(spectrum::safeDivide(ramp, zero)[last] == static_cast<float>(last + 1) / 2.0F);
}

};  // namespace
