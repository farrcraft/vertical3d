/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/F82.h>
#include <moya/libmoya/kernel/bsdf/Ggx.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/material/Albedos.h>

#include <cmath>
#include <cstdint>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Scattering.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace spectrum = v3d::moya::kernel::spectrum;

/** A spectrum of one value at every wavelength. **/
spectrum::SampledSpectrum flat(float value) {
    return spectrum::SampledSpectrum(value);
}

/** A direction of a polar cosine about z, turned about it. **/
glm::vec3 towards(float cosine, float turn) {
    const float across = std::sqrt(1.0F - (cosine * cosine));
    return glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine };
}

/**
 * A BSDF of one conductor of a roughness and an anisotropy, its reflectance at normal incidence
 * and its F82 tint flat, shaded about +z with the albedo table the build integrated.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the roughness, then the colours
bsdf::Bsdf metal(float roughness, float anisotropy, float normal = 1.0F, float tint = 1.0F) {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    bsdf::Closure closure;
    closure.model = bsdf::Model::Conductor;
    closure.weight = flat(1.0F);
    closure.alpha = bsdf::Ggx::alphas(roughness, anisotropy);
    closure.fresnel = bsdf::F82 { .normal = flat(normal), .tint = flat(tint) };
    out.add(closure);
    return out;
}

// the F82 factor is the reflectance at normal incidence there, the tint times Schlick's at the
// tint's cosine, and one at grazing. Its mean is twice its integral times the cosine
BOOST_AUTO_TEST_CASE(moya_conductor_the_fresnel_factor_meets_its_reflectance_and_tint_test) {
    const bsdf::F82 fresnel { .normal = flat(0.6F), .tint = flat(0.8F) };
    BOOST_TEST(fresnel(1.0F)[0] == 0.6F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(fresnel(bsdf::kTintCosine)[0] == 0.8F * bsdf::F82::schlick(0.6F, bsdf::kTintCosine), boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(fresnel(0.0F)[0] == 1.0F, boost::test_tools::tolerance(1.0e-6F));
    constexpr int kSteps = 20000;
    double integral = 0.0;
    for (int i = 0; i < kSteps; ++i) {
        const float cosine = (static_cast<float>(i) + 0.5F) / kSteps;
        integral += 2.0 * static_cast<double>(fresnel(cosine)[0] * cosine) / kSteps;
    }
    BOOST_TEST(fresnel.average()[0] == integral, boost::test_tools::tolerance(1.0e-4));
    const bsdf::F82 white;
    BOOST_TEST(white(0.3F)[0] == 1.0F);
    BOOST_TEST(white.average()[0] == 1.0F);
}

// OpenPBR's alphas: the square of the roughness, the same along both axes with no anisotropy,
// and stretched along the tangent with it, keeping their product's root at the alpha's
BOOST_AUTO_TEST_CASE(moya_conductor_anisotropy_stretches_the_alphas_along_the_tangent_test) {
    const glm::vec2 even = bsdf::Ggx::alphas(0.5F, 0.0F);
    BOOST_TEST(even.x == 0.25F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(even.y == 0.25F, boost::test_tools::tolerance(1.0e-6F));
    const glm::vec2 stretched = bsdf::Ggx::alphas(0.5F, 0.6F);
    BOOST_TEST(stretched.x > 0.25F);
    BOOST_TEST(stretched.y < 0.25F);
    BOOST_TEST(stretched.y == 0.4F * stretched.x, boost::test_tools::tolerance(1.0e-5F));
    // a highlight is wider along the tangent: light from just off the mirror direction along x is
    // reflected more than from as far off it along y
    const bsdf::Bsdf anisotropic = metal(0.5F, 0.6F);
    const glm::vec3 outgoing { 0.0F, 0.0F, 1.0F };
    const float alongTangent = anisotropic.evaluate(outgoing, glm::normalize(glm::vec3 { 0.4F, 0.0F, 1.0F }))[0];
    const float alongBitangent = anisotropic.evaluate(outgoing, glm::normalize(glm::vec3 { 0.0F, 0.4F, 1.0F }))[0];
    BOOST_TEST(alongTangent > 1.5F * alongBitangent);
}

// a conductor's samples follow its density, rough or anisotropic, from near and far off the normal
BOOST_AUTO_TEST_CASE(moya_conductor_samples_follow_their_density_test) {
    std::uint32_t seed = 101;
    for (const float roughness : { 0.3F, 0.6F, 1.0F }) {
        for (const float anisotropy : { 0.0F, 0.7F }) {
            for (const float cosine : { 0.9F, 0.3F }) {
                BOOST_TEST_CONTEXT("roughness " << roughness << ", anisotropy " << anisotropy << ", cosine " << cosine) {
                    const bsdf::Bsdf conductor = metal(roughness, anisotropy, 0.7F, 0.9F);
                    const Fit fit = densityFit(conductor, towards(cosine, 0.6F), 500000, seed++);
                    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
                }
            }
        }
    }
}

// a conductor's value is the same with its two directions swapped, coloured, rough and stretched
BOOST_AUTO_TEST_CASE(moya_conductor_is_reciprocal_test) {
    const bsdf::Bsdf conductor = metal(0.45F, 0.5F, 0.3F, 0.7F);
    std::uint32_t seed = 111;
    for (int i = 0; i < 500; ++i) {
        const glm::vec3 one = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        const glm::vec3 other = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        const float forward = conductor.evaluate(one, other)[0];
        const float backward = conductor.evaluate(other, one)[0];
        BOOST_TEST(forward == backward, boost::test_tools::tolerance(1.0e-4F));
    }
}

// a white conductor keeps all its energy at every roughness and angle: the compensation gives back
// what a single scattering loses. Without it, a rough one loses much at a grazing angle
BOOST_AUTO_TEST_CASE(moya_conductor_a_white_furnace_keeps_all_its_energy_test) {
    std::uint32_t seed = 121;
    for (const float roughness : { 0.1F, 0.3F, 0.5F, 0.7F, 1.0F }) {
        for (const float cosine : { 0.95F, 0.6F, 0.25F }) {
            BOOST_TEST_CONTEXT("roughness " << roughness << ", cosine " << cosine) {
                BOOST_TEST(furnace(metal(roughness, 0.0F), towards(cosine, 1.1F), 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.01F));
            }
        }
    }
    const kernel::bsdf::Albedos albedos = v3d::moya::material::ggxAlbedos();
    BOOST_TEST(albedos.at(0.25F, 1.0F) < 0.8F);
    BOOST_TEST(albedos.at(1.0F, 1.0e-4F) == 1.0F);
    BOOST_TEST(albedos.averageAt(1.0F) < albedos.averageAt(0.25F));
}

// a coloured conductor's samples weigh what its value says it reflects: their mean is its value
// times the cosine, integrated over the hemisphere, and it is less than a white one's
BOOST_AUTO_TEST_CASE(moya_conductor_a_coloured_furnace_is_its_values_integral_test) {
    const bsdf::Bsdf gold = metal(0.6F, 0.0F, 0.6F, 0.9F);
    const glm::vec3 outgoing = towards(0.7F, 0.4F);
    constexpr int kSide = 600;
    constexpr float kPi = 3.14159265F;
    double integral = 0.0;
    for (int row = 0; row < kSide; ++row) {
        const float polar = 0.5F * kPi * (static_cast<float>(row) + 0.5F) / kSide;
        for (int column = 0; column < kSide; ++column) {
            const float turn = 2.0F * kPi * (static_cast<float>(column) + 0.5F) / kSide;
            const glm::vec3 incoming { std::sin(polar) * std::cos(turn), std::sin(polar) * std::sin(turn), std::cos(polar) };
            integral += static_cast<double>(gold.evaluate(outgoing, incoming)[0] * incoming.z * std::sin(polar)) * (0.5 * kPi / kSide) * (2.0 * kPi / kSide);
        }
    }
    const float sampled = furnace(gold, outgoing, 200000, 131);
    BOOST_TEST(sampled == integral, boost::test_tools::tolerance(0.01));
    BOOST_TEST(sampled < furnace(metal(0.6F, 0.0F), outgoing, 100000, 133));
}

// a smooth conductor is a mirror of its Fresnel factor: its sample is specular, and at normal
// incidence it reflects its reflectance there. A rough one's samples are glossy
BOOST_AUTO_TEST_CASE(moya_conductor_a_smooth_conductor_is_specular_test) {
    const bsdf::Bsdf smooth = metal(0.0F, 0.0F, 0.55F, 0.8F);
    const bsdf::Sample straight = smooth.sample({ 0.0F, 0.0F, 1.0F }, { 0.3F, 0.6F });
    BOOST_TEST_REQUIRE(straight.found());
    BOOST_TEST(straight.specular);
    BOOST_TEST((straight.lobe == bsdf::Lobe::Specular));
    BOOST_TEST(straight.value[0] == 0.55F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(glm::distance(straight.incoming, glm::vec3 { 0.0F, 0.0F, 1.0F }) < 1.0e-6F);
    BOOST_TEST(smooth.evaluate({ 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F, 1.0F })[0] == 0.0F);

    const bsdf::Sample rough = metal(0.5F, 0.0F).sample(towards(0.8F, 0.0F), { 0.3F, 0.6F });
    BOOST_TEST_REQUIRE(rough.found());
    BOOST_TEST(!rough.specular);
    BOOST_TEST((rough.lobe == bsdf::Lobe::Glossy));
}

};  // namespace

};  // namespace v3d::moya::tracing
