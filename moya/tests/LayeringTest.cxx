/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/F82.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fresnel.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/scattering/OrenNayar.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/bsdf/scattering/Specular.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/material/Albedos.h>

#include <cmath>
#include <cstdint>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
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

/** An empty BSDF shaded about +z, with the albedo tables the build integrated. **/
bsdf::Bsdf shaded() {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    return out;
}

/** A BSDF of one closure, of a weight of one. **/
bsdf::Bsdf alone(bsdf::Closure closure) {
    bsdf::Bsdf out = shaded();
    closure.weight = spectrum::SampledSpectrum(1.0F);
    out.add(closure);
    return out;
}

/** An Oren-Nayar closure of a grey and a roughness. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the grey, then the roughness
bsdf::Closure rough(float grey, float roughness) {
    bsdf::Closure out;
    out.model = bsdf::Model::OrenNayar;
    out.colour = spectrum::SampledSpectrum(grey);
    out.roughness = roughness;
    return out;
}

/** A white specular reflection of a roughness and an index. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the roughness, then the index
bsdf::Closure specular(float roughness, float ior) {
    bsdf::Closure out;
    out.model = bsdf::Model::Specular;
    out.alpha = bsdf::scattering::Ggx::alphas(roughness, 0.0F);
    out.ior = ior;
    return out;
}

/**
 * The wavelengths a layered surface's colours are made spectra at. The cases compare its value at
 * the first, and a grey is the same at every one.
 **/
spectrum::SampledWavelengths wavelengths() {
    return spectrum::SampledWavelengths::visible(0.5F);
}

/** The BSDF of OpenPBR's parameters for light leaving along outgoing. **/
bsdf::Bsdf layered(const kernel::surface::OpenPbr & surface, const glm::vec3 & outgoing) {
    bsdf::Bsdf out = shaded();
    bsdf::Layering::add(out, surface, colour::acescgTable(), wavelengths(), outgoing.z);
    return out;
}

/** A grey surface of OpenPBR's parameters, with a specular layer of a roughness. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the grey, then the roughnesses
kernel::surface::OpenPbr greySurface(float grey, float roughness, float diffuseRoughness) {
    kernel::surface::OpenPbr out;
    out.baseColour = glm::vec3 { grey };
    out.specularRoughness = roughness;
    out.baseDiffuseRoughness = diffuseRoughness;
    return out;
}

// with no roughness the Oren-Nayar lobe is Lambert's: its reflectance over pi in every direction
BOOST_AUTO_TEST_CASE(moya_layering_a_smooth_oren_nayar_lobe_is_lamberts_test) {
    const bsdf::Bsdf lobe = alone(rough(0.6F, 0.0F));
    std::uint32_t seed = 301;
    for (int i = 0; i < 100; ++i) {
        const glm::vec3 one = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        const glm::vec3 other = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        BOOST_TEST(lobe.evaluate(one, other)[0] == 0.6F * bsdf::scattering::kInversePi, boost::test_tools::tolerance(1.0e-5F));
    }
    BOOST_TEST(bsdf::scattering::OrenNayar::albedo(0.4F, 0.0F) == 1.0F, boost::test_tools::tolerance(1.0e-6F));
}

// the single scattering's albedo is its value times the cosine integrated over the hemisphere,
// white, and its mean is twice its integral times the cosine
BOOST_AUTO_TEST_CASE(moya_layering_the_oren_nayar_albedo_is_its_integral_test) {
    constexpr int kSide = 400;
    constexpr float kPi = 3.14159265F;
    for (const float cosine : { 0.9F, 0.5F, 0.15F }) {
        BOOST_TEST_CONTEXT("cosine " << cosine) {
            const glm::vec3 outgoing = towards(cosine, 0.3F);
            double integral = 0.0;
            for (int row = 0; row < kSide; ++row) {
                const float polar = 0.5F * kPi * (static_cast<float>(row) + 0.5F) / kSide;
                for (int column = 0; column < kSide; ++column) {
                    const float turn = 2.0F * kPi * (static_cast<float>(column) + 0.5F) / kSide;
                    const glm::vec3 incoming { std::sin(polar) * std::cos(turn), std::sin(polar) * std::sin(turn), std::cos(polar) };
                    const float s = glm::dot(outgoing, incoming) - (outgoing.z * incoming.z);
                    const float overT = s > 0.0F ? s / std::fmax(outgoing.z, incoming.z) : s;
                    const float value = bsdf::scattering::OrenNayar::scale(0.8F) * (1.0F + (0.8F * overT)) / kPi;
                    integral += static_cast<double>(value * incoming.z * std::sin(polar)) * (0.5 * kPi / kSide) * (2.0 * kPi / kSide);
                }
            }
            BOOST_TEST(bsdf::scattering::OrenNayar::albedo(cosine, 0.8F) == integral, boost::test_tools::tolerance(2.0e-3));
        }
    }
    constexpr int kSteps = 20000;
    double mean = 0.0;
    for (int i = 0; i < kSteps; ++i) {
        const float cosine = (static_cast<float>(i) + 0.5F) / kSteps;
        mean += 2.0 * static_cast<double>(bsdf::scattering::OrenNayar::albedo(cosine, 0.8F) * cosine) / kSteps;
    }
    BOOST_TEST(bsdf::scattering::OrenNayar::average(0.8F) == mean, boost::test_tools::tolerance(1.0e-3));
}

// an Oren-Nayar lobe follows its density and is reciprocal. A white one keeps all its energy, and
// a grey one reflects its single scattering's albedo plus what the bounces between facets return
BOOST_AUTO_TEST_CASE(moya_layering_an_oren_nayar_lobe_keeps_its_energy_test) {
    std::uint32_t seed = 311;
    const Fit fit = densityFit(alone(rough(0.5F, 0.7F)), towards(0.5F, 0.2F), 300000, seed++);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
    const bsdf::Bsdf grey = alone(rough(0.5F, 0.9F));
    for (int i = 0; i < 300; ++i) {
        const glm::vec3 one = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        const glm::vec3 other = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        BOOST_TEST(grey.evaluate(one, other)[0] == grey.evaluate(other, one)[0], boost::test_tools::tolerance(1.0e-4F));
    }
    for (const float roughness : { 0.4F, 1.0F }) {
        for (const float cosine : { 0.9F, 0.25F }) {
            BOOST_TEST_CONTEXT("roughness " << roughness << ", cosine " << cosine) {
                BOOST_TEST(furnace(alone(rough(1.0F, roughness)), towards(cosine, 1.0F), 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.01F));
                const float expected = bsdf::scattering::OrenNayar::reflectance(rough(0.5F, roughness), cosine)[0];
                BOOST_TEST(expected < 0.5F);
                BOOST_TEST(furnace(alone(rough(0.5F, roughness)), towards(cosine, 1.0F), 200000, seed++) == expected, boost::test_tools::tolerance(0.01F));
            }
        }
    }
}

// a specular reflection follows its density, is reciprocal, and reflects the albedo the layering
// scales the base by. A smooth one reflects ((eta - 1) / (eta + 1)) squared at normal incidence
BOOST_AUTO_TEST_CASE(moya_layering_a_specular_reflection_reflects_its_albedo_test) {
    std::uint32_t seed = 321;
    const Fit fit = densityFit(alone(specular(0.4F, 1.5F)), towards(0.6F, 0.2F), 500000, seed++);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
    const bsdf::Bsdf reflection = alone(specular(0.5F, 1.7F));
    for (int i = 0; i < 300; ++i) {
        const glm::vec3 one = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        const glm::vec3 other = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        BOOST_TEST(reflection.evaluate(one, other)[0] == reflection.evaluate(other, one)[0], boost::test_tools::tolerance(1.0e-4F));
    }
    const kernel::bsdf::Albedos albedos = v3d::moya::material::ggxAlbedos();
    for (const float roughness : { 0.2F, 0.5F, 0.9F }) {
        for (const float ior : { 1.5F, 2.0F }) {
            for (const float cosine : { 0.9F, 0.3F }) {
                BOOST_TEST_CONTEXT("roughness " << roughness << ", ior " << ior << ", cosine " << cosine) {
                    const bsdf::Closure closure = specular(roughness, ior);
                    const float expected = bsdf::scattering::Specular::albedo(albedos, cosine, std::sqrt(closure.alpha.x * closure.alpha.y), ior);
                    BOOST_TEST(furnace(alone(closure), towards(cosine, 1.0F), 200000, seed++) == expected, boost::test_tools::tolerance(0.02F));
                }
            }
        }
    }
    const bsdf::Sample straight = alone(specular(0.0F, 1.5F)).sample({ 0.0F, 0.0F, 1.0F }, { 0.5F, 0.5F }, 0.5F);
    BOOST_TEST_REQUIRE(straight.found());
    BOOST_TEST(straight.specular);
    BOOST_TEST(straight.value[0] == 0.04F, boost::test_tools::tolerance(1.0e-4F));
}

// OpenPBR's limits: a metalness of one is the conductor of its base colour and specular colour,
// a specular weight of zero leaves the diffuse base alone, and a coat of no weight changes nothing
BOOST_AUTO_TEST_CASE(moya_layering_openpbrs_limits_are_its_lobes_test) {
    const glm::vec3 outgoing = towards(0.7F, 0.4F);
    kernel::surface::OpenPbr metal = greySurface(0.6F, 0.4F, 0.0F);
    metal.baseMetalness = 1.0F;
    metal.specularColour = glm::vec3 { 0.9F };
    bsdf::Closure conductor;
    conductor.model = bsdf::Model::Conductor;
    conductor.alpha = bsdf::scattering::Ggx::alphas(0.4F, 0.0F);
    conductor.fresnel = bsdf::scattering::F82 { .normal = spectrum::reflectance(colour::acescgTable(), glm::vec3 { 0.6F }, wavelengths()),
        .tint = spectrum::reflectance(colour::acescgTable(), glm::vec3 { 0.9F }, wavelengths()) };
    const bsdf::Bsdf asMetal = layered(metal, outgoing);
    const bsdf::Bsdf asConductor = alone(conductor);

    kernel::surface::OpenPbr unlit = greySurface(0.6F, 0.4F, 0.5F);
    unlit.specularWeight = 0.0F;
    bsdf::Closure diffuse = rough(0.0F, 0.5F);
    diffuse.colour = spectrum::reflectance(colour::acescgTable(), glm::vec3 { 0.6F }, wavelengths());
    const bsdf::Bsdf asDiffuse = layered(unlit, outgoing);
    const bsdf::Bsdf asLobe = alone(diffuse);

    const kernel::surface::OpenPbr plain = greySurface(0.6F, 0.4F, 0.5F);
    kernel::surface::OpenPbr uncoated = plain;
    uncoated.coatColour = glm::vec3 { 0.2F };
    uncoated.coatRoughness = 0.7F;
    uncoated.coatIor = 2.0F;
    const bsdf::Bsdf asPlain = layered(plain, outgoing);
    const bsdf::Bsdf asUncoated = layered(uncoated, outgoing);

    std::uint32_t seed = 331;
    for (int i = 0; i < 200; ++i) {
        const glm::vec3 incoming = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        BOOST_TEST(asMetal.evaluate(outgoing, incoming)[0] == asConductor.evaluate(outgoing, incoming)[0], boost::test_tools::tolerance(1.0e-5F));
        BOOST_TEST(asDiffuse.evaluate(outgoing, incoming)[0] == asLobe.evaluate(outgoing, incoming)[0], boost::test_tools::tolerance(1.0e-5F));
        BOOST_TEST(asPlain.evaluate(outgoing, incoming)[0] == asUncoated.evaluate(outgoing, incoming)[0]);
    }
    BOOST_TEST(asMetal.count == 1U);
    BOOST_TEST(asDiffuse.count == 1U);
}

// a white OpenPBR surface keeps all its energy, coated or not, metal or not, at every angle. A grey
// base reflects what each layer's albedo lets through to it, and the coat's darkening only darkens
BOOST_AUTO_TEST_CASE(moya_layering_a_white_furnace_keeps_each_layers_energy_test) {
    std::uint32_t seed = 341;
    const kernel::bsdf::Albedos albedos = v3d::moya::material::ggxAlbedos();
    for (const float cosine : { 0.9F, 0.5F, 0.2F }) {
        BOOST_TEST_CONTEXT("cosine " << cosine) {
            const glm::vec3 outgoing = towards(cosine, 0.8F);
            kernel::surface::OpenPbr white = greySurface(1.0F, 0.5F, 0.5F);
            BOOST_TEST(furnace(layered(white, outgoing), outgoing, 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));
            white.coatWeight = 1.0F;
            white.coatRoughness = 0.3F;
            BOOST_TEST(furnace(layered(white, outgoing), outgoing, 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));
            white.baseMetalness = 0.5F;
            BOOST_TEST(furnace(layered(white, outgoing), outgoing, 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));

            kernel::surface::OpenPbr grey = greySurface(0.4F, 0.5F, 0.6F);
            const bsdf::Closure layer = specular(0.5F, 1.5F);
            const float under = bsdf::scattering::Specular::albedo(albedos, cosine, std::sqrt(layer.alpha.x * layer.alpha.y), 1.5F);
            const float base = bsdf::scattering::OrenNayar::reflectance(rough(spectrum::reflectance(colour::acescgTable(), glm::vec3 { 0.4F }, wavelengths())[0], 0.6F), cosine)[0];
            const float uncoated = under + ((1.0F - under) * base);
            BOOST_TEST(furnace(layered(grey, outgoing), outgoing, 200000, seed++) == uncoated, boost::test_tools::tolerance(0.02F));

            grey.coatWeight = 1.0F;
            grey.coatDarkening = 0.0F;
            const float coat = bsdf::scattering::Specular::albedo(albedos, cosine, bsdf::scattering::kLeast, 1.6F);
            const float relative = bsdf::scattering::Specular::albedo(albedos, cosine, std::sqrt(layer.alpha.x * layer.alpha.y), 1.6F / 1.5F);
            const float coated = coat + ((1.0F - coat) * (relative + ((1.0F - relative) * base)));
            const float bright = furnace(layered(grey, outgoing), outgoing, 200000, seed++);
            BOOST_TEST(bright == coated, boost::test_tools::tolerance(0.02F));
            grey.coatDarkening = 1.0F;
            BOOST_TEST(furnace(layered(grey, outgoing), outgoing, 200000, seed++) < 0.97F * bright);
        }
    }
}

};  // namespace

};  // namespace v3d::moya::tracing
