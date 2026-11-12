/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Fresnel.h>
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
#include <glm/vec3.hpp>

#include "Scattering.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace spectrum = v3d::moya::kernel::spectrum;

/** A direction of a polar cosine about z, turned about it. A negative cosine lies below. **/
glm::vec3 towards(float cosine, float turn) {
    const float across = std::sqrt(1.0F - (cosine * cosine));
    return glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine };
}

/**
 * A BSDF of one white dielectric of a roughness, an index and an anisotropy, thin-walled or not,
 * shaded about +z with the albedo tables the build integrated.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the roughness, then the index
bsdf::Bsdf glass(float roughness, float ior, bool thin = false, float anisotropy = 0.0F) {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    bsdf::Closure closure;
    closure.model = bsdf::Model::Dielectric;
    closure.weight = spectrum::SampledSpectrum(1.0F);
    closure.alpha = bsdf::Ggx::alphas(roughness, anisotropy);
    closure.ior = ior;
    closure.thin = thin;
    out.add(closure);
    return out;
}

// Fresnel's equations meet their closed forms: ((eta - 1) / (eta + 1)) squared at normal
// incidence, the perpendicular half alone at Brewster's angle, and all of it beyond the critical
// angle and at grazing. A thin slab reflects 2R / (1 + R) at normal incidence
BOOST_AUTO_TEST_CASE(moya_dielectric_fresnel_meets_its_closed_forms_test) {
    for (const float eta : { 1.33F, 1.5F, 2.4F }) {
        BOOST_TEST_CONTEXT("eta " << eta) {
            const float normal = ((eta - 1.0F) / (eta + 1.0F)) * ((eta - 1.0F) / (eta + 1.0F));
            BOOST_TEST(bsdf::Fresnel::reflected(1.0F, eta) == normal, boost::test_tools::tolerance(1.0e-5F));
            BOOST_TEST(bsdf::Fresnel::reflected(1.0F, 1.0F / eta) == normal, boost::test_tools::tolerance(1.0e-5F));
            const float brewster = 1.0F / std::sqrt(1.0F + (eta * eta));
            const float through = std::sqrt(1.0F - ((1.0F - (brewster * brewster)) / (eta * eta)));
            const float perpendicular = (brewster - (eta * through)) / (brewster + (eta * through));
            BOOST_TEST(bsdf::Fresnel::reflected(brewster, eta) == 0.5F * perpendicular * perpendicular, boost::test_tools::tolerance(1.0e-4F));
            const float critical = std::sqrt(1.0F - (1.0F / (eta * eta)));
            BOOST_TEST(bsdf::Fresnel::reflected(critical - 0.01F, 1.0F / eta) == 1.0F);
            BOOST_TEST(bsdf::Fresnel::reflected(critical + 0.01F, 1.0F / eta) < 1.0F);
            BOOST_TEST(bsdf::Fresnel::reflected(0.0F, eta) == 1.0F, boost::test_tools::tolerance(1.0e-5F));
            BOOST_TEST(bsdf::Fresnel::slab(1.0F, eta) == 2.0F * normal / (1.0F + normal), boost::test_tools::tolerance(1.0e-5F));
        }
    }
    BOOST_TEST(bsdf::Fresnel::reflected(0.7F, 1.0F) == 0.0F);
}

// a refraction keeps Snell's law: the sines of the two angles are in the ratio of the indices, on
// the far side of the normal, and none is found beyond the critical angle
BOOST_AUTO_TEST_CASE(moya_dielectric_a_refraction_keeps_snells_law_test) {
    const glm::vec3 normal { 0.0F, 0.0F, 1.0F };
    const glm::vec3 from = towards(0.6F, 0.3F);
    glm::vec3 bent { 0.0F };
    BOOST_TEST_REQUIRE(bsdf::Fresnel::refract(from, normal, 1.5F, bent));
    BOOST_TEST(bent.z < 0.0F);
    BOOST_TEST(glm::length(bent) == 1.0F, boost::test_tools::tolerance(1.0e-5F));
    const float sineIn = std::sqrt(1.0F - (from.z * from.z));
    const float sineOut = std::sqrt(1.0F - (bent.z * bent.z));
    BOOST_TEST(sineIn == 1.5F * sineOut, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(bent.x * from.x < 0.0F);
    BOOST_TEST(!bsdf::Fresnel::refract(towards(0.3F, 0.0F), normal, 1.0F / 1.5F, bent));
}

// a rough dielectric's samples follow its density over both hemispheres, entering and leaving,
// anisotropic, and as a thin slab
BOOST_AUTO_TEST_CASE(moya_dielectric_samples_follow_their_density_test) {
    std::uint32_t seed = 201;
    for (const bool thin : { false, true }) {
        for (const float roughness : { 0.3F, 0.7F }) {
            for (const float cosine : { 0.9F, 0.35F, -0.8F, -0.4F }) {
                BOOST_TEST_CONTEXT("thin " << thin << ", roughness " << roughness << ", cosine " << cosine) {
                    const Fit fit = densityFit(glass(roughness, 1.5F, thin), towards(cosine, 0.6F), 500000, seed++);
                    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
                }
            }
        }
    }
    const Fit stretched = densityFit(glass(0.5F, 1.5F, false, 0.6F), towards(0.7F, 1.2F), 500000, seed++);
    BOOST_TEST(stretched.passes(), "chi-squared " << stretched.statistic << " over " << stretched.freedom);
}

// a dielectric's value over the square of the index on outgoing's side is the same with its two
// directions swapped, reflected or transmitted. A thin slab's is the same unweighted
BOOST_AUTO_TEST_CASE(moya_dielectric_is_reciprocal_across_the_interface_test) {
    constexpr float kIor = 1.5F;
    const bsdf::Bsdf interface = glass(0.45F, kIor);
    const bsdf::Bsdf slab = glass(0.45F, kIor, true);
    const auto squared = [](const glm::vec3 & direction) { return direction.z > 0.0F ? 1.0F : kIor * kIor; };
    std::uint32_t seed = 211;
    int crossed = 0;
    for (int i = 0; i < 1000; ++i) {
        const float sign = next(seed) < 0.5F ? -1.0F : 1.0F;
        const glm::vec3 one = towards(sign * (0.05F + (0.95F * next(seed))), 6.2831853F * next(seed));
        const glm::vec3 other = towards((next(seed) < 0.5F ? -1.0F : 1.0F) * (0.05F + (0.95F * next(seed))), 6.2831853F * next(seed));
        crossed += one.z * other.z < 0.0F ? 1 : 0;
        const float forward = interface.evaluate(one, other)[0] / squared(one);
        const float backward = interface.evaluate(other, one)[0] / squared(other);
        BOOST_TEST(forward == backward, boost::test_tools::tolerance(2.0e-3F));
        BOOST_TEST(slab.evaluate(one, other)[0] == slab.evaluate(other, one)[0], boost::test_tools::tolerance(2.0e-3F));
    }
    BOOST_TEST(crossed > 300);
    BOOST_TEST(interface.evaluate(towards(0.8F, 0.0F), towards(-0.7F, 3.0F))[0] > 0.0F);
}

// a white rough dielectric keeps all the energy that reaches it, entering and leaving, as a thin
// slab, and at an index between the table's points. Without the compensation a rough one loses
// much at a grazing angle
BOOST_AUTO_TEST_CASE(moya_dielectric_a_white_furnace_keeps_all_its_energy_test) {
    std::uint32_t seed = 221;
    for (const float ior : { 1.5F, 1.33F, 2.2F }) {
        for (const float roughness : { 0.2F, 0.5F, 1.0F }) {
            for (const float cosine : { 0.9F, 0.3F, -0.9F, -0.3F }) {
                BOOST_TEST_CONTEXT("ior " << ior << ", roughness " << roughness << ", cosine " << cosine) {
                    const float crossing = cosine > 0.0F ? ior * ior : 1.0F / (ior * ior);
                    BOOST_TEST(furnace(glass(roughness, ior), towards(cosine, 1.1F), 200000, seed++, crossing) == 1.0F, boost::test_tools::tolerance(0.02F));
                    BOOST_TEST(furnace(glass(roughness, ior, true), towards(cosine, 1.1F), 100000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));
                }
            }
        }
    }
    const kernel::bsdf::Albedos albedos = v3d::moya::material::ggxAlbedos();
    BOOST_TEST(albedos.dielectricAt(0.2F, 1.0F, 1.5F, false) < 0.9F);
    BOOST_TEST(albedos.dielectricAverageAt(1.0e-4F, 1.5F, true) == 1.0F, boost::test_tools::tolerance(1.0e-3F));
}

// a smooth dielectric is specular: at normal incidence it reflects ((eta - 1) / (eta + 1)) squared
// and transmits the rest straight on, over eta squared as radiance. From inside beyond the critical
// angle it reflects everything. A thin slab passes light through unbent
BOOST_AUTO_TEST_CASE(moya_dielectric_a_smooth_dielectric_is_specular_test) {
    const bsdf::Bsdf smooth = glass(0.0F, 1.5F);
    const glm::vec3 straight { 0.0F, 0.0F, 1.0F };
    const bsdf::Sample reflected = smooth.sample(straight, { 0.5F, 0.5F }, 0.01F);
    BOOST_TEST_REQUIRE(reflected.found());
    BOOST_TEST(reflected.specular);
    BOOST_TEST((reflected.lobe == bsdf::Lobe::Specular));
    BOOST_TEST(reflected.value[0] == 0.04F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(reflected.density == 0.04F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(glm::distance(reflected.incoming, straight) < 1.0e-6F);

    const bsdf::Sample through = smooth.sample(straight, { 0.5F, 0.5F }, 0.5F);
    BOOST_TEST_REQUIRE(through.found());
    BOOST_TEST(through.specular);
    BOOST_TEST((through.lobe == bsdf::Lobe::Transmission));
    BOOST_TEST(through.value[0] == 0.96F / 2.25F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(through.density == 0.96F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(glm::distance(through.incoming, -straight) < 1.0e-6F);
    BOOST_TEST(smooth.evaluate(straight, -straight)[0] == 0.0F);

    const bsdf::Sample trapped = smooth.sample(towards(-0.5F, 0.4F), { 0.5F, 0.5F }, 0.99F);
    BOOST_TEST_REQUIRE(trapped.found());
    BOOST_TEST(trapped.incoming.z < 0.0F);
    BOOST_TEST(trapped.value[0] == 1.0F);

    const glm::vec3 slanted = towards(0.6F, 0.4F);
    const bsdf::Sample unbent = glass(0.0F, 1.5F, true).sample(slanted, { 0.5F, 0.5F }, 0.9F);
    BOOST_TEST_REQUIRE(unbent.found());
    BOOST_TEST(glm::distance(unbent.incoming, -slanted) < 1.0e-6F);
    BOOST_TEST(unbent.value[0] == 1.0F - bsdf::Fresnel::slab(0.6F, 1.5F), boost::test_tools::tolerance(1.0e-5F));

    const bsdf::Sample passed = glass(0.5F, 1.0F).sample(slanted, { 0.5F, 0.5F }, 0.5F);
    BOOST_TEST_REQUIRE(passed.found());
    BOOST_TEST(passed.specular);
    BOOST_TEST(glm::distance(passed.incoming, -slanted) < 1.0e-6F);
}

// a rough dielectric's sample that crosses the surface is a transmission, and one that stays on
// its side glossy
BOOST_AUTO_TEST_CASE(moya_dielectric_a_rough_sample_names_its_lobe_test) {
    const bsdf::Bsdf rough = glass(0.4F, 1.5F);
    const glm::vec3 outgoing = towards(0.8F, 0.0F);
    int transmitted = 0;
    int reflected = 0;
    std::uint32_t seed = 231;
    for (int i = 0; i < 200; ++i) {
        const bsdf::Sample drawn = rough.sample(outgoing, { next(seed), next(seed) }, next(seed));
        if (!drawn.found()) {
            continue;
        }
        BOOST_TEST(!drawn.specular);
        if (drawn.incoming.z < 0.0F) {
            BOOST_TEST((drawn.lobe == bsdf::Lobe::Transmission));
            ++transmitted;
        } else {
            BOOST_TEST((drawn.lobe == bsdf::Lobe::Glossy));
            ++reflected;
        }
    }
    BOOST_TEST(transmitted > 100);
    BOOST_TEST(reflected > 0);
}

};  // namespace

};  // namespace v3d::moya::tracing
