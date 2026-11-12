/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/scattering/F82.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fresnel.h>
#include <moya/libmoya/kernel/bsdf/scattering/ThinFilm.h>
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

/** Wavelengths drawn from the middle of the visible range. **/
spectrum::SampledWavelengths wavelengths() {
    return spectrum::SampledWavelengths::visible(0.37F);
}

/** The wavelength a film is evaluated at for a place of a path's wavelengths. **/
float lambdaAt(const spectrum::SampledWavelengths & drawn, int index) {
#ifdef MOYA_RGB
    static_cast<void>(drawn);
    return bsdf::scattering::kRgbWavelengths[index];
#else
    return drawn.lambda[index];
#endif
}

/** A direction of a polar cosine about z, turned about it. **/
glm::vec3 towards(float cosine, float turn) {
    const float across = std::sqrt(1.0F - (cosine * cosine));
    return glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine };
}

/** The BSDF of OpenPBR's parameters for light leaving along outgoing. **/
bsdf::Bsdf layered(const kernel::surface::OpenPbr & surface, const glm::vec3 & outgoing) {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    bsdf::Layering::add(out, surface, colour::acescgTable(), wavelengths(), outgoing.z);
    return out;
}

// a quarter-wave film of the square root of the base's index reflects nothing at normal incidence,
// and a half-wave film reflects what the bare base does. A vanishing film is the bare base's
// Fresnel factor at every angle
BOOST_AUTO_TEST_CASE(moya_thin_film_meets_its_closed_forms_test) {
    const spectrum::SampledWavelengths drawn = wavelengths();
    constexpr float kBase = 1.69F;
    const float film = std::sqrt(kBase);
    for (int i = 0; i < spectrum::kSamples; ++i) {
        BOOST_TEST_CONTEXT("wavelength " << lambdaAt(drawn, i)) {
            const bsdf::scattering::ThinFilm quarter { .thickness = lambdaAt(drawn, i) / (4.0F * film), .ior = film };
            BOOST_TEST(quarter.overDielectric(1.0F, kBase, drawn)[i] < 1.0e-6F);
            const bsdf::scattering::ThinFilm half { .thickness = lambdaAt(drawn, i) / (2.0F * film), .ior = film };
            BOOST_TEST(half.overDielectric(1.0F, kBase, drawn)[i] == bsdf::scattering::Fresnel::reflected(1.0F, kBase), boost::test_tools::tolerance(1.0e-4F));
        }
    }
    const bsdf::scattering::ThinFilm vanishing { .thickness = 1.0e-4F, .ior = 1.3F };
    for (const float cosine : { 1.0F, 0.8F, 0.5F, 0.2F }) {
        BOOST_TEST_CONTEXT("cosine " << cosine) {
            BOOST_TEST(vanishing.overDielectric(cosine, 1.5F, drawn)[0] == bsdf::scattering::Fresnel::reflected(cosine, 1.5F), boost::test_tools::tolerance(1.0e-4F));
        }
    }
    const bsdf::scattering::ThinFilm none;
    BOOST_TEST(!none.present());
}

// a film's colours come from the spectrum: a film half a micrometre thick reflects a different
// share at each of a path's wavelengths, and each lies within [0, 1], over a dielectric or a metal
BOOST_AUTO_TEST_CASE(moya_thin_film_reflects_by_the_wavelength_test) {
    const spectrum::SampledWavelengths drawn = wavelengths();
    const bsdf::scattering::ThinFilm film { .thickness = 500.0F, .ior = 1.4F };
    const bsdf::scattering::F82 gold { .normal = spectrum::SampledSpectrum(0.8F), .tint = spectrum::SampledSpectrum(0.9F) };
    for (const float cosine : { 0.9F, 0.4F }) {
        const spectrum::SampledSpectrum over = film.overDielectric(cosine, 1.5F, drawn);
        const spectrum::SampledSpectrum metal = film.overConductor(cosine, gold.normal, [&gold](float inside) { return gold(inside); }, drawn);
        float least = 1.0F;
        float most = 0.0F;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            BOOST_TEST(over[i] >= 0.0F);
            BOOST_TEST(over[i] <= 1.0F);
            BOOST_TEST(metal[i] >= 0.0F);
            BOOST_TEST(metal[i] <= 1.0F);
            least = std::fmin(least, over[i]);
            most = std::fmax(most, over[i]);
        }
        BOOST_TEST(most - least > 0.01F);
    }
}

// a film of no weight changes nothing, and one of half weight is halfway to a whole one. A white
// surface under a film keeps its energy
BOOST_AUTO_TEST_CASE(moya_thin_film_is_mixed_in_by_its_weight_test) {
    const glm::vec3 outgoing = towards(0.7F, 0.3F);
    kernel::surface::OpenPbr plain;
    plain.baseColour = glm::vec3 { 0.5F };
    plain.baseMetalness = 0.5F;
    plain.specularRoughness = 0.4F;
    kernel::surface::OpenPbr unfilmed = plain;
    unfilmed.thinFilmThickness = 0.3F;
    unfilmed.thinFilmIor = 1.7F;
    kernel::surface::OpenPbr whole = unfilmed;
    whole.thinFilmWeight = 1.0F;
    kernel::surface::OpenPbr half = unfilmed;
    half.thinFilmWeight = 0.5F;
    const bsdf::Bsdf asPlain = layered(plain, outgoing);
    const bsdf::Bsdf asUnfilmed = layered(unfilmed, outgoing);
    const bsdf::Bsdf asWhole = layered(whole, outgoing);
    const bsdf::Bsdf asHalf = layered(half, outgoing);
    std::uint32_t seed = 501;
    int changed = 0;
    for (int i = 0; i < 100; ++i) {
        const glm::vec3 incoming = towards(0.05F + (0.95F * next(seed)), 6.2831853F * next(seed));
        const float bare = asPlain.evaluate(outgoing, incoming)[0];
        BOOST_TEST(bare == asUnfilmed.evaluate(outgoing, incoming)[0]);
        const float filmed = asWhole.evaluate(outgoing, incoming)[0];
        changed += std::fabs(filmed - bare) > 1.0e-4F * bare ? 1 : 0;
        BOOST_TEST(asHalf.evaluate(outgoing, incoming)[0] > std::fmin(bare, filmed) - 1.0e-6F);
        BOOST_TEST(asHalf.evaluate(outgoing, incoming)[0] < std::fmax(bare, filmed) + 1.0e-6F);
    }
    BOOST_TEST(changed > 50);

    kernel::surface::OpenPbr white;
    white.baseColour = glm::vec3 { 1.0F };
    white.specularRoughness = 0.4F;
    white.thinFilmWeight = 1.0F;
    BOOST_TEST(furnace(layered(white, outgoing), outgoing, 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));
}

};  // namespace

};  // namespace v3d::moya::tracing
