/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fuzz.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/material/Albedos.h>

#include <cmath>
#include <cstdint>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

#include "Scattering.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace spectrum = v3d::moya::kernel::spectrum;

/** A direction of a polar cosine about z, turned about it. **/
glm::vec3 towards(float cosine, float turn) {
    const float across = std::sqrt(1.0F - (cosine * cosine));
    return glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine };
}

/** A BSDF shaded about +z with the albedo tables the build integrated, and no closures. **/
bsdf::Bsdf shaded() {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    return out;
}

/** A BSDF of one white fuzz of a roughness. **/
bsdf::Bsdf fuzz(float roughness) {
    bsdf::Bsdf out = shaded();
    bsdf::Closure closure;
    closure.model = bsdf::Model::Fuzz;
    closure.weight = spectrum::SampledSpectrum(1.0F);
    closure.roughness = roughness;
    out.add(closure);
    return out;
}

/** The BSDF of OpenPBR's parameters for light leaving along outgoing. **/
bsdf::Bsdf layered(const kernel::surface::OpenPbr & surface, const glm::vec3 & outgoing) {
    bsdf::Bsdf out = shaded();
    bsdf::Layering::add(out, surface, colour::acescgTable(), spectrum::SampledWavelengths::visible(0.5F), outgoing.z);
    return out;
}

// fuzz's samples follow its density, from near the normal and from grazing, smooth and rough
BOOST_AUTO_TEST_CASE(moya_fuzz_samples_follow_their_density_test) {
    std::uint32_t seed = 401;
    for (const float roughness : { 0.2F, 0.6F, 1.0F }) {
        for (const float cosine : { 0.9F, 0.4F, -0.7F }) {
            BOOST_TEST_CONTEXT("roughness " << roughness << ", cosine " << cosine) {
                const Fit fit = densityFit(fuzz(roughness), towards(cosine, 0.5F), 400000, seed++);
                BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
            }
        }
    }
}

// fuzz reflects its albedo: its value times the cosine integrates to it over the hemisphere, and
// its samples' mean weight is it. The albedo lies within [0, 1] wherever the fits hold
BOOST_AUTO_TEST_CASE(moya_fuzz_reflects_its_albedo_test) {
    constexpr int kSide = 500;
    constexpr float kPi = 3.14159265F;
    std::uint32_t seed = 411;
    for (const float roughness : { 0.3F, 0.8F }) {
        for (const float cosine : { 0.85F, 0.3F }) {
            BOOST_TEST_CONTEXT("roughness " << roughness << ", cosine " << cosine) {
                const bsdf::Bsdf lobe = fuzz(roughness);
                const glm::vec3 outgoing = towards(cosine, 0.7F);
                double integral = 0.0;
                for (int row = 0; row < kSide; ++row) {
                    const float polar = 0.5F * kPi * (static_cast<float>(row) + 0.5F) / kSide;
                    for (int column = 0; column < kSide; ++column) {
                        const float turn = 2.0F * kPi * (static_cast<float>(column) + 0.5F) / kSide;
                        const glm::vec3 incoming { std::sin(polar) * std::cos(turn), std::sin(polar) * std::sin(turn), std::cos(polar) };
                        integral += static_cast<double>(lobe.evaluate(outgoing, incoming)[0] * incoming.z * std::sin(polar)) * (0.5 * kPi / kSide) *
                            (2.0 * kPi / kSide);
                    }
                }
                const float albedo = bsdf::scattering::Fuzz::albedo(cosine, roughness);
                BOOST_TEST(integral == albedo, boost::test_tools::tolerance(0.01));
                BOOST_TEST(furnace(lobe, outgoing, 200000, seed++) == albedo, boost::test_tools::tolerance(0.01F));
            }
        }
    }
    for (int i = 0; i <= 20; ++i) {
        for (int j = 1; j <= 20; ++j) {
            const float albedo = bsdf::scattering::Fuzz::albedo(static_cast<float>(i) / 20.0F, static_cast<float>(j) / 20.0F);
            BOOST_TEST(albedo >= 0.0F);
            BOOST_TEST(albedo <= 1.0F);
        }
    }
}

// fuzz of no weight changes nothing. White fuzz over a white surface keeps all its energy, and over
// a grey one it reflects its albedo, the surface reflecting the rest of what it lets through
BOOST_AUTO_TEST_CASE(moya_fuzz_lies_over_the_surface_test) {
    std::uint32_t seed = 421;
    const glm::vec3 outgoing = towards(0.6F, 0.2F);
    kernel::surface::OpenPbr plain;
    plain.baseColour = glm::vec3 { 0.4F };
    kernel::surface::OpenPbr unfuzzed = plain;
    unfuzzed.fuzzColour = glm::vec3 { 0.3F };
    unfuzzed.fuzzRoughness = 0.9F;
    const bsdf::Bsdf asPlain = layered(plain, outgoing);
    const bsdf::Bsdf asUnfuzzed = layered(unfuzzed, outgoing);
    for (int i = 0; i < 100; ++i) {
        const glm::vec3 incoming = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        BOOST_TEST(asPlain.evaluate(outgoing, incoming)[0] == asUnfuzzed.evaluate(outgoing, incoming)[0]);
    }

    kernel::surface::OpenPbr white;
    white.baseColour = glm::vec3 { 1.0F };
    white.fuzzWeight = 1.0F;
    white.fuzzRoughness = 0.5F;
    BOOST_TEST(furnace(layered(white, outgoing), outgoing, 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));

    const float under = furnace(asPlain, outgoing, 200000, seed++);
    kernel::surface::OpenPbr fuzzed = plain;
    fuzzed.fuzzWeight = 1.0F;
    fuzzed.fuzzRoughness = 0.5F;
    const float albedo = bsdf::scattering::Fuzz::albedo(0.6F, 0.5F);
    BOOST_TEST(furnace(layered(fuzzed, outgoing), outgoing, 200000, seed++) == albedo + ((1.0F - albedo) * under), boost::test_tools::tolerance(0.02F));
}

};  // namespace

};  // namespace v3d::moya::tracing
