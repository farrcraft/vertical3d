/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Daylight.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbSigmoid.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <array>
#include <cmath>
#include <functional>
#include <limits>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Chart.h"

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

/** The ACEScg colour a reflectance's sigmoid gives under ACES white. **/
glm::vec3 reflected(const spectrum::RgbSigmoid & sigmoid) {
    return spectrum::acescgFromXyz(exact([&sigmoid](float lambda) { return sigmoid(lambda) * spectrum::acesWhite(lambda); }));
}

/** A linear Rec. 709 colour in ACEScg, with Bradford adaptation from D65 to ACES white. **/
glm::vec3 fromRec709(const glm::vec3 & rgb) {
    constexpr std::array<std::array<float, 3>, 3> kRec709ToAcescg { { { 0.6130974024F, 0.3395231462F, 0.0473794514F },
        { 0.0701937225F, 0.9163538791F, 0.0134523985F }, { 0.0206155929F, 0.1095697729F, 0.8698146342F } } };
    glm::vec3 out(0.0F);
    for (int i = 0; i < 3; ++i) {
        out[i] = (kRec709ToAcescg[i][0] * rgb.x) + (kRec709ToAcescg[i][1] * rgb.y) + (kRec709ToAcescg[i][2] * rgb.z);
    }
    return out;
}

/**
 * Checks that a colour's reflectance gives the colour back within an absolute difference on each
 * channel. The fit minimises a perceptual difference, so a dark channel's error is small in
 * absolute terms and may be large relative to the channel.
 **/
void checkRoundTrip(const glm::vec3 & colour, float difference) {
    const glm::vec3 back = reflected(v3d::moya::colour::acescgTable()(colour));
    BOOST_TEST_CONTEXT("ACEScg " << colour.x << ", " << colour.y << ", " << colour.z << " came back " << back.x << ", " << back.y << ", " << back.z) {
        for (int i = 0; i < 3; ++i) {
            BOOST_TEST(std::abs(back[i] - colour[i]) < difference);
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_rgb_daylight_at_6504_kelvin_is_d65_test) {
    for (int lambda = 300; lambda <= 830; lambda += 5) {
        BOOST_TEST_CONTEXT(lambda << " nm") {
            BOOST_TEST(std::abs(spectrum::daylight(static_cast<float>(lambda), 6504.0F) - spectrum::d65(static_cast<float>(lambda))) < 0.05F);
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_rgb_aces_white_is_acescg_white_test) {
    const glm::vec3 xyz = exact([](float lambda) { return spectrum::acesWhite(lambda); });
    BOOST_TEST(xyz.y == 1.0F, boost::test_tools::tolerance(0.0001F));
    const float total = xyz.x + xyz.y + xyz.z;
    BOOST_TEST(xyz.x / total == 0.32168F, boost::test_tools::tolerance(0.00005F));
    BOOST_TEST(xyz.y / total == 0.33767F, boost::test_tools::tolerance(0.00005F));
    const glm::vec3 rgb = spectrum::acescgFromXyz(xyz);
    for (int i = 0; i < 3; ++i) {
        BOOST_TEST(rgb[i] == 1.0F, boost::test_tools::tolerance(0.0002F));
    }
}

BOOST_AUTO_TEST_CASE(moya_rgb_the_acescg_matrices_are_inverses_and_hold_its_primaries_test) {
    for (const glm::vec3 & colour : { glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(0.2F, 0.7F, 0.4F) }) {
        const glm::vec3 back = spectrum::acescgFromXyz(spectrum::acescgToXyz(colour));
        for (int i = 0; i < 3; ++i) {
            BOOST_TEST(back[i] == colour[i], boost::test_tools::tolerance(0.00001F));
        }
    }
    const glm::vec3 red = spectrum::acescgToXyz(glm::vec3(1, 0, 0));
    BOOST_TEST(red.x / (red.x + red.y + red.z) == 0.713F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(red.y / (red.x + red.y + red.z) == 0.293F, boost::test_tools::tolerance(0.0001F));
}

BOOST_AUTO_TEST_CASE(moya_rgb_a_grey_is_a_constant_spectrum_test) {
    const spectrum::RgbTable table = v3d::moya::colour::acescgTable();
    for (const float grey : { 0.0F, 0.18F, 0.5F, 1.0F }) {
        const spectrum::RgbSigmoid sigmoid = table(glm::vec3(grey));
        for (const float lambda : { 360.0F, 550.0F, 830.0F }) {
            BOOST_TEST(sigmoid(lambda) == grey, boost::test_tools::tolerance(0.00001F));
        }
    }
}

/**
 * A sigmoid of an infinite argument is exactly zero or one, so black and white are exactly zero
 * and one at every wavelength, with no tolerance.
 *
 * Covers the base: 49d103a4 added RgbSigmoid and its infinite argument. Checked by removing the
 * infinite branch from RgbSigmoid's operator(), which gives NaN and fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_rgb_black_and_white_are_exact_test) {
    const float infinity = std::numeric_limits<float>::infinity();
    const spectrum::RgbSigmoid up { .c2 = infinity };
    const spectrum::RgbSigmoid down { .c2 = -infinity };
    const spectrum::RgbTable table = v3d::moya::colour::acescgTable();
    const spectrum::RgbSigmoid black = table(glm::vec3(0.0F));
    const spectrum::RgbSigmoid white = table(glm::vec3(1.0F));
    for (const float lambda : { 360.0F, 550.0F, 830.0F }) {
        BOOST_TEST(up(lambda) == 1.0F);
        BOOST_TEST(down(lambda) == 0.0F);
        BOOST_TEST(black(lambda) == 0.0F);
        BOOST_TEST(white(lambda) == 1.0F);
    }
}

BOOST_AUTO_TEST_CASE(moya_rgb_srgbs_primaries_and_secondaries_return_to_their_colour_test) {
    for (const glm::vec3 & colour : { glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 1, 0), glm::vec3(0, 1, 1),
             glm::vec3(1, 0, 1), glm::vec3(0.5F, 0, 0), glm::vec3(0, 0.5F, 0), glm::vec3(0, 0, 0.5F) }) {
        checkRoundTrip(fromRec709(colour), 0.005F);
    }
}

BOOST_AUTO_TEST_CASE(moya_rgb_a_chart_of_moderate_colours_returns_to_its_colour_test) {
    for (const glm::vec3 & colour : v3d::moya::chart::colours()) {
        checkRoundTrip(colour, 0.005F);
    }
}

BOOST_AUTO_TEST_CASE(moya_rgb_a_reflectance_lies_between_zero_and_one_test) {
    const spectrum::RgbTable table = v3d::moya::colour::acescgTable();
    for (const glm::vec3 & colour : { glm::vec3(1, 0, 0), glm::vec3(0.9F, 0.1F, 0.6F), glm::vec3(2, -1, 0.5F), glm::vec3(0.01F, 0.02F, 0.03F) }) {
        const spectrum::RgbSigmoid sigmoid = table(colour);
        for (int lambda = 360; lambda <= 830; lambda += 10) {
            const float value = sigmoid(static_cast<float>(lambda));
            BOOST_TEST(value >= 0.0F);
            BOOST_TEST(value <= 1.0F);
        }
    }
}

// a light brighter than one is its colour's reflectance scaled up, times ACES white
BOOST_AUTO_TEST_CASE(moya_rgb_a_light_has_its_colour_test) {
    const spectrum::RgbTable table = v3d::moya::colour::acescgTable();
    const glm::vec3 colour = fromRec709(glm::vec3(2.0F, 1.0F, 0.5F));
    const float scale = 2.0F * colour.x;
    const spectrum::RgbSigmoid sigmoid = table(colour / scale);
    const glm::vec3 back = spectrum::acescgFromXyz(exact([&](float lambda) { return scale * sigmoid(lambda) * spectrum::acesWhite(lambda); }));
    for (int i = 0; i < 3; ++i) {
        BOOST_TEST(back[i] == colour[i], boost::test_tools::tolerance(0.01F));
    }
    const spectrum::SampledWavelengths wavelengths = spectrum::SampledWavelengths::visible(0.4F);
    BOOST_TEST(spectrum::unbounded(table, glm::vec3(0.0F), wavelengths).black());
#ifndef MOYA_RGB
    // illuminant() gives the same spectrum at a path's wavelengths
    const spectrum::SampledSpectrum light = spectrum::illuminant(table, colour, wavelengths);
    for (int i = 0; i < spectrum::kSamples; ++i) {
        const float lambda = wavelengths.lambda[i];
        BOOST_TEST(light[i] == scale * sigmoid(lambda) * spectrum::acesWhite(lambda), boost::test_tools::tolerance(0.0001F));
    }
#endif
}

// an RGB build's illuminants are these colours, so both builds check them against the tables
BOOST_AUTO_TEST_CASE(moya_rgb_d65_and_illuminant_a_have_their_acescg_colours_test) {
    const glm::vec3 d65 = spectrum::acescgFromXyz(exact([](float lambda) { return spectrum::d65(lambda); }));
    const glm::vec3 warm = spectrum::acescgFromXyz(exact([](float lambda) { return spectrum::illuminantA(lambda); }));
    for (int i = 0; i < 3; ++i) {
        BOOST_TEST(spectrum::kD65Acescg[i] == d65[i], boost::test_tools::tolerance(0.0001F));
        BOOST_TEST(spectrum::kIlluminantAAcescg[i] == warm[i], boost::test_tools::tolerance(0.0001F));
    }
}

#ifdef MOYA_RGB
// an RGB build carries a colour as ACEScg's channels, with no spectrum behind them
BOOST_AUTO_TEST_CASE(moya_rgb_an_rgb_build_carries_acescg_channels_test) {
    const spectrum::RgbTable table = v3d::moya::colour::acescgTable();
    const spectrum::SampledWavelengths wavelengths = spectrum::SampledWavelengths::visible(0.4F);
    const spectrum::SampledSpectrum surface = spectrum::reflectance(table, glm::vec3(0.3F, 1.5F, -0.2F), wavelengths);
    BOOST_TEST(surface[0] == 0.3F);
    BOOST_TEST(surface[1] == 1.0F);
    BOOST_TEST(surface[2] == 0.0F);
    const spectrum::SampledSpectrum light = spectrum::illuminant(table, glm::vec3(2.0F, 1.0F, -0.5F), wavelengths);
    BOOST_TEST(light[0] == 2.0F);
    BOOST_TEST(light[1] == 1.0F);
    BOOST_TEST(light[2] == 0.0F);
    const glm::vec3 xyz = spectrum::toXyz(spectrum::SampledSpectrum(1.0F), wavelengths);
    BOOST_TEST(xyz.y == 1.0F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(spectrum::d65(wavelengths)[1] == spectrum::kD65Acescg[1]);
}
#else
BOOST_AUTO_TEST_CASE(moya_rgb_a_paths_reflectance_is_the_sigmoid_at_its_wavelengths_test) {
    const spectrum::RgbTable table = v3d::moya::colour::acescgTable();
    const glm::vec3 colour(0.3F, 0.6F, 0.2F);
    const spectrum::SampledWavelengths wavelengths = spectrum::SampledWavelengths::visible(0.7F);
    const spectrum::SampledSpectrum value = spectrum::reflectance(table, colour, wavelengths);
    for (int i = 0; i < spectrum::kSamples; ++i) {
        BOOST_TEST(value[i] == table(colour)(wavelengths.lambda[i]));
    }
}
#endif

};  // namespace
