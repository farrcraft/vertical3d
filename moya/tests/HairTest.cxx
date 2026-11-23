/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/bsdf/AtSurface.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fibre.h>
#include <moya/libmoya/kernel/bsdf/scattering/Hair.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Hair.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Scattering.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace scattering = v3d::moya::kernel::bsdf::scattering;
namespace spectrum = v3d::moya::kernel::spectrum;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace material = v3d::moya::material;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
using CpuDevice = v3d::moya::device::CpuDevice;

constexpr double kPi = 3.14159265358979323846;

/*
 * pbrt-v4's HairBxDF, as its source gives it, in double precision: the oracle the model is held
 * to. It takes pbrt-v4's parameters: an offset, an index, a grey absorption, the longitudinal and
 * azimuthal roughnesses beta_m and beta_n, and the scales' tilt in degrees.
 */
struct Pbrt {
    double h;
    double eta;
    double sigma;
    double v[4];
    double s;
    double sin2kAlpha[3];
    double cos2kAlpha[3];

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - pbrt-v4's order of its parameters
    Pbrt(double offset, double ior, double absorption, double betaM, double betaN, double alpha) : h(offset), eta(ior), sigma(absorption) {
        v[0] = std::pow((0.726 * betaM) + (0.812 * betaM * betaM) + (3.7 * std::pow(betaM, 20.0)), 2.0);
        v[1] = 0.25 * v[0];
        v[2] = 4.0 * v[0];
        v[3] = v[2];
        s = 0.626657069 * ((0.265 * betaN) + (1.194 * betaN * betaN) + (5.372 * std::pow(betaN, 22.0)));
        sin2kAlpha[0] = std::sin(alpha * kPi / 180.0);
        cos2kAlpha[0] = safeSqrt(1.0 - (sin2kAlpha[0] * sin2kAlpha[0]));
        for (int i = 1; i < 3; ++i) {
            sin2kAlpha[i] = 2.0 * cos2kAlpha[i - 1] * sin2kAlpha[i - 1];
            cos2kAlpha[i] = (cos2kAlpha[i - 1] * cos2kAlpha[i - 1]) - (sin2kAlpha[i - 1] * sin2kAlpha[i - 1]);
        }
    }

    static double safeSqrt(double x) {
        return std::sqrt(std::max(x, 0.0));
    }

    static double safeAsin(double x) {
        return std::asin(std::clamp(x, -1.0, 1.0));
    }

    static double i0(double x) {
        double val = 0.0;
        double x2i = 1.0;
        double ifact = 1.0;
        double i4 = 1.0;
        for (int i = 0; i < 10; ++i) {
            if (i > 1) {
                ifact *= i;
            }
            val += x2i / (i4 * ifact * ifact);
            x2i *= x * x;
            i4 *= 4.0;
        }
        return val;
    }

    static double logI0(double x) {
        if (x > 12.0) {
            return x + (0.5 * (-std::log(2.0 * kPi) + std::log(1.0 / x) + (1.0 / (8.0 * x))));
        }
        return std::log(i0(x));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - pbrt-v4's order of its parameters
    static double mp(double cosThetaI, double cosThetaO, double sinThetaI, double sinThetaO, double v) {
        const double cosines = cosThetaI * cosThetaO / v;
        const double sines = sinThetaI * sinThetaO / v;
        return v <= 0.1 ? std::exp(logI0(cosines) - sines - (1.0 / v) + 0.6931 + std::log(1.0 / (2.0 * v)))
                        : (std::exp(-sines) * i0(cosines)) / (std::sinh(1.0 / v) * 2.0 * v);
    }

    static double frDielectric(double cosThetaI, double eta) {
        cosThetaI = std::clamp(cosThetaI, -1.0, 1.0);
        if (cosThetaI < 0.0) {
            eta = 1.0 / eta;
            cosThetaI = -cosThetaI;
        }
        const double sin2ThetaI = 1.0 - (cosThetaI * cosThetaI);
        const double sin2ThetaT = sin2ThetaI / (eta * eta);
        if (sin2ThetaT >= 1.0) {
            return 1.0;
        }
        const double cosThetaT = safeSqrt(1.0 - sin2ThetaT);
        const double rParl = ((eta * cosThetaI) - cosThetaT) / ((eta * cosThetaI) + cosThetaT);
        const double rPerp = (cosThetaI - (eta * cosThetaT)) / (cosThetaI + (eta * cosThetaT));
        return ((rParl * rParl) + (rPerp * rPerp)) / 2.0;
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the transmittance
    std::array<double, 4> ap(double cosThetaO, double t) const {
        std::array<double, 4> out {};
        const double cosGammaO = safeSqrt(1.0 - (h * h));
        const double fresnel = frDielectric(cosThetaO * cosGammaO, eta);
        out[0] = fresnel;
        out[1] = (1.0 - fresnel) * (1.0 - fresnel) * t;
        out[2] = out[1] * t * fresnel;
        if (1.0 - (t * fresnel) != 0.0) {
            out[3] = out[2] * fresnel * t / (1.0 - (t * fresnel));
        }
        return out;
    }

    static double phi(int lobe, double gammaO, double gammaT) {
        return (2.0 * lobe * gammaT) - (2.0 * gammaO) + (lobe * kPi);
    }

    static double logistic(double x, double s) {
        x = std::abs(x);
        return std::exp(-x / s) / (s * (1.0 + std::exp(-x / s)) * (1.0 + std::exp(-x / s)));
    }

    static double logisticCdf(double x, double s) {
        return 1.0 / (1.0 + std::exp(-x / s));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - pbrt-v4's order of its parameters
    static double np(double phiValue, int lobe, double s, double gammaO, double gammaT) {
        double dphi = phiValue - phi(lobe, gammaO, gammaT);
        while (dphi > kPi) {
            dphi -= 2.0 * kPi;
        }
        while (dphi < -kPi) {
            dphi += 2.0 * kPi;
        }
        return logistic(dphi, s) / (logisticCdf(kPi, s) - logisticCdf(-kPi, s));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - a sine and cosine, then the tilted
    void tilted(int lobe, double sinThetaO, double cosThetaO, double & sinThetapO, double & cosThetapO) const {
        if (lobe == 0) {
            sinThetapO = (sinThetaO * cos2kAlpha[1]) - (cosThetaO * sin2kAlpha[1]);
            cosThetapO = (cosThetaO * cos2kAlpha[1]) + (sinThetaO * sin2kAlpha[1]);
        } else if (lobe == 1) {
            sinThetapO = (sinThetaO * cos2kAlpha[0]) + (cosThetaO * sin2kAlpha[0]);
            cosThetapO = (cosThetaO * cos2kAlpha[0]) - (sinThetaO * sin2kAlpha[0]);
        } else if (lobe == 2) {
            sinThetapO = (sinThetaO * cos2kAlpha[2]) + (cosThetaO * sin2kAlpha[2]);
            cosThetapO = (cosThetaO * cos2kAlpha[2]) - (sinThetaO * sin2kAlpha[2]);
        } else {
            sinThetapO = sinThetaO;
            cosThetapO = cosThetaO;
        }
        cosThetapO = std::abs(cosThetapO);
    }

    /** f(), and the density PDF() gives, for two directions. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the value, then the density
    void evaluate(const glm::dvec3 & wo, const glm::dvec3 & wi, double & value, double & density) const {
        const double sinThetaO = wo.x;
        const double cosThetaO = safeSqrt(1.0 - (sinThetaO * sinThetaO));
        const double phiO = std::atan2(wo.z, wo.y);
        const double gammaO = safeAsin(h);
        const double sinThetaI = wi.x;
        const double cosThetaI = safeSqrt(1.0 - (sinThetaI * sinThetaI));
        const double phiI = std::atan2(wi.z, wi.y);
        const double sinThetaT = sinThetaO / eta;
        const double cosThetaT = safeSqrt(1.0 - (sinThetaT * sinThetaT));
        const double etap = safeSqrt((eta * eta) - (sinThetaO * sinThetaO)) / cosThetaO;
        const double sinGammaT = h / etap;
        const double cosGammaT = safeSqrt(1.0 - (sinGammaT * sinGammaT));
        const double gammaT = safeAsin(sinGammaT);
        const double t = std::exp(-sigma * (2.0 * cosGammaT / cosThetaT));
        const double phiValue = phiI - phiO;
        const std::array<double, 4> attenuation = ap(cosThetaO, t);
        const double sum = attenuation[0] + attenuation[1] + attenuation[2] + attenuation[3];
        value = 0.0;
        density = 0.0;
        for (int lobe = 0; lobe < 3; ++lobe) {
            double sinThetapO = 0.0;
            double cosThetapO = 0.0;
            tilted(lobe, sinThetaO, cosThetaO, sinThetapO, cosThetapO);
            const double longitudinal = mp(cosThetaI, cosThetapO, sinThetaI, sinThetapO, v[lobe]);
            const double azimuthal = np(phiValue, lobe, s, gammaO, gammaT);
            value += longitudinal * attenuation[lobe] * azimuthal;
            density += longitudinal * (attenuation[lobe] / sum) * azimuthal;
        }
        const double rest = mp(cosThetaI, cosThetaO, sinThetaI, sinThetaO, v[3]) / (2.0 * kPi);
        value += rest * attenuation[3];
        density += rest * attenuation[3] / sum;
        if (std::abs(wi.z) > 0.0) {
            value /= std::abs(wi.z);
        }
    }

    /** The sum of the attenuations, which is the albedo, for an outgoing longitudinal sine. **/
    double albedo(double sinThetaO) const {
        const double cosThetaO = safeSqrt(1.0 - (sinThetaO * sinThetaO));
        const double sinThetaT = sinThetaO / eta;
        const double cosThetaT = safeSqrt(1.0 - (sinThetaT * sinThetaT));
        const double etap = safeSqrt((eta * eta) - (sinThetaO * sinThetaO)) / cosThetaO;
        const double sinGammaT = h / etap;
        const double cosGammaT = safeSqrt(1.0 - (sinGammaT * sinGammaT));
        const std::array<double, 4> attenuation = ap(cosThetaO, std::exp(-sigma * (2.0 * cosGammaT / cosThetaT)));
        return attenuation[0] + attenuation[1] + attenuation[2] + attenuation[3];
    }
};

/** The beta_m roughness of pbrt-v4 as MaterialX's longitudinal variance of R. **/
float varianceOf(float betaM) {
    const float deviation = (0.726F * betaM) + (0.812F * betaM * betaM) + (3.7F * std::pow(betaM, 20.0F));
    return deviation * deviation;
}

/** The beta_n roughness of pbrt-v4 as MaterialX's azimuthal roughness. **/
float azimuthalOf(float betaN) {
    return (0.265F * betaN) + (1.194F * betaN * betaN) + (5.372F * std::pow(betaN, 22.0F));
}

/**
 * MaterialX's parameters of a white fibre that matches pbrt-v4's of an index, roughnesses and a
 * tilt in degrees.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - pbrt-v4's order
kernel::surface::Hair matching(float ior, float betaM, float betaN, float alpha) {
    const float variance = varianceOf(betaM);
    const float azimuthal = azimuthalOf(betaN);
    kernel::surface::Hair out;
    out.ior = ior;
    out.roughnessR = glm::vec2 { variance, azimuthal };
    out.roughnessTT = glm::vec2 { 0.25F * variance, azimuthal };
    out.roughnessTRT = glm::vec2 { 4.0F * variance, azimuthal };
    // pbrt-v4's tilt turns R's outgoing direction by minus twice it
    out.cuticleAngle = 0.5F - (alpha / 180.0F);
    return out;
}

/** A fibre of MaterialX's parameters, met at an offset, of a grey absorption. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the offset, then the absorption
scattering::Fibre fibreOf(const kernel::surface::Hair & hair, float offset, float absorption = 0.0F) {
    scattering::Fibre out = scattering::Hair::fibre(hair, colour::acescgTable(), spectrum::SampledWavelengths::visible(0.5F), offset);
    out.absorption = spectrum::SampledSpectrum(absorption);
    return out;
}

/** A BSDF of one hair closure, whose fibre runs along x and whose normal is z. **/
bsdf::Bsdf haired(const scattering::Fibre & fibre) {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame {};
    out.coatFrame = out.frame;
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    out.fibre = fibre;
    bsdf::Closure closure;
    closure.model = bsdf::Model::Hair;
    closure.weight = spectrum::SampledSpectrum(1.0F);
    closure.roughness = std::sqrt(fibre.variance[0]);
    out.add(closure);
    return out;
}

/** A direction of a longitudinal sine along x and an azimuth in y and z. **/
glm::vec3 fibreward(float sine, float azimuth) {
    const float cosine = std::sqrt(1.0F - (sine * sine));
    return glm::vec3 { sine, cosine * std::cos(azimuth), cosine * std::sin(azimuth) };
}

// the model's value and density match pbrt-v4's published implementation at fixed directions,
// across offsets, indices, absorptions, roughnesses and tilts
BOOST_AUTO_TEST_CASE(moya_hair_matches_pbrt_v4_test) {
    struct Case {
        float h;
        float eta;
        float sigma;
        float betaM;
        float betaN;
        float alpha;
    };
    const Case cases[] = { { 0.3F, 1.55F, 0.5F, 0.3F, 0.3F, 2.0F }, { -0.7F, 1.4F, 1.2F, 0.25F, 0.6F, -3.0F }, { 0.0F, 1.6F, 0.0F, 0.4F, 0.2F, 5.0F },
        { 0.9F, 1.55F, 0.1F, 0.15F, 0.7F, 0.0F } };
    std::uint32_t seed = 1701;
    for (const Case & each : cases) {
        const Pbrt oracle(each.h, each.eta, each.sigma, each.betaM, each.betaN, each.alpha);
        const scattering::Fibre fibre = fibreOf(matching(each.eta, each.betaM, each.betaN, each.alpha), each.h, each.sigma);
        for (int i = 0; i < 40; ++i) {
            const glm::vec3 outgoing = fibreward((1.8F * next(seed)) - 0.9F, 6.2831853F * next(seed));
            const glm::vec3 incoming = fibreward((1.8F * next(seed)) - 0.9F, 6.2831853F * next(seed));
            double value = 0.0;
            double density = 0.0;
            oracle.evaluate(glm::dvec3(outgoing), glm::dvec3(incoming), value, density);
            BOOST_TEST_CONTEXT("h " << each.h << ", beta_m " << each.betaM << ", case " << i) {
                const float got = scattering::Hair::evaluate(fibre, outgoing, incoming)[0];
                BOOST_TEST(std::fabs(static_cast<double>(got) - value) <= 2.0e-3 * std::max(1.0, value), got << " against " << value);
                const float drawn = scattering::Hair::density(fibre, outgoing, incoming);
                BOOST_TEST(std::fabs(static_cast<double>(drawn) - density) <= 2.0e-3 * std::max(1.0, density), drawn << " against " << density);
            }
        }
    }
}

// the model's samples follow its density, at several roughnesses, offsets and directions
BOOST_AUTO_TEST_CASE(moya_hair_samples_follow_their_density_test) {
    struct Case {
        float betaM;
        float betaN;
        float h;
        float alpha;
    };
    const Case cases[] = { { 0.3F, 0.3F, 0.4F, 2.0F }, { 0.4F, 0.7F, -0.8F, 0.0F }, { 0.25F, 0.5F, 0.0F, -4.0F } };
    std::uint32_t seed = 1711;
    for (const Case & each : cases) {
        const bsdf::Bsdf lobe = haired(fibreOf(matching(1.55F, each.betaM, each.betaN, each.alpha), each.h, 0.3F));
        for (const float sine : { 0.0F, 0.5F, -0.85F }) {
            BOOST_TEST_CONTEXT("beta_m " << each.betaM << ", beta_n " << each.betaN << ", h " << each.h << ", sine " << sine) {
                const Fit fit = densityFit(lobe, fibreward(sine, 1.3F), 400000, seed++);
                BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
            }
        }
    }
}

// a white fibre that absorbs nothing returns a white furnace's light, tilted or not. One that
// absorbs returns the sum of its attenuations, as pbrt-v4 gives them
BOOST_AUTO_TEST_CASE(moya_hair_keeps_a_furnaces_light_test) {
    std::uint32_t seed = 1721;
    for (const float betaM : { 0.2F, 0.4F }) {
        for (const float offset : { -0.6F, 0.1F, 0.95F }) {
            for (const float alpha : { 0.0F, 3.0F }) {
                BOOST_TEST_CONTEXT("beta_m " << betaM << ", h " << offset << ", alpha " << alpha) {
                    const bsdf::Bsdf white = haired(fibreOf(matching(1.55F, betaM, 0.5F, alpha), offset));
                    BOOST_TEST(furnace(white, fibreward(0.3F, 0.9F), 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));
                    BOOST_TEST(furnace(white, fibreward(-0.75F, 2.5F), 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.02F));
                }
            }
        }
    }
    const Pbrt oracle(0.4, 1.55, 0.8, 0.3, 0.5, 2.0);
    const bsdf::Bsdf absorbing = haired(fibreOf(matching(1.55F, 0.3F, 0.5F, 2.0F), 0.4F, 0.8F));
    for (const float sine : { 0.0F, 0.6F }) {
        BOOST_TEST(furnace(absorbing, fibreward(sine, 0.4F), 200000, seed++) == static_cast<float>(oracle.albedo(sine)), boost::test_tools::tolerance(0.02F));
    }
    BOOST_TEST(scattering::Hair::albedo(absorbing.fibre)[0] == static_cast<float>(oracle.albedo(0.0)), boost::test_tools::tolerance(1.0e-4F));
}

// a sample from TT is a transmission and one from any other lobe a glossy reflection, so a fibre
// of TT alone transmits every sample and one without it none. A light sample's part takes the
// kind of the lobe that carries most of its light
BOOST_AUTO_TEST_CASE(moya_hair_counts_its_bounces_by_their_lobe_test) {
    kernel::surface::Hair through = matching(1.55F, 0.3F, 0.4F, 0.0F);
    through.tintR = glm::vec3 { 0.0F };
    through.tintTRT = glm::vec3 { 0.0F };
    kernel::surface::Hair reflecting = matching(1.55F, 0.3F, 0.4F, 0.0F);
    reflecting.tintTT = glm::vec3 { 0.0F };
    const bsdf::Bsdf transmits = haired(fibreOf(through, 0.2F));
    const bsdf::Bsdf reflects = haired(fibreOf(reflecting, 0.2F));
    const glm::vec3 outgoing = fibreward(0.2F, 1.0F);
    std::uint32_t seed = 1731;
    std::size_t transmitted = 0;
    std::size_t glossy = 0;
    for (int i = 0; i < 2000; ++i) {
        const glm::vec2 u { next(seed), next(seed) };
        const float choice = next(seed);
        const bsdf::Sample one = transmits.sample(outgoing, u, choice);
        const bsdf::Sample other = reflects.sample(outgoing, u, choice);
        transmitted += one.found() && one.lobe == bsdf::Lobe::Transmission ? 1U : 0U;
        glossy += other.found() && other.lobe == bsdf::Lobe::Glossy ? 1U : 0U;
        if (one.found()) {
            BOOST_TEST((transmits.part(0, outgoing, one.incoming).lobe == bsdf::Lobe::Transmission));
        }
        if (other.found()) {
            BOOST_TEST((reflects.part(0, outgoing, other.incoming).lobe == bsdf::Lobe::Glossy));
        }
        BOOST_TEST(!one.specular);
        BOOST_TEST(!one.diffuse);
    }
    BOOST_TEST(transmitted == 2000U);
    BOOST_TEST(glossy == 2000U);
}

// MaterialX's cuticle angle a tilts R's peak to minus outgoing's longitudinal angle less twice
// (a pi - pi / 2), as its GLSL implementation turns incoming. A half tilts nothing
BOOST_AUTO_TEST_CASE(moya_hair_tilts_by_materialxs_cuticle_angle_test) {
    for (const float angle : { 0.5F, 0.53F, 0.46F }) {
        kernel::surface::Hair hair;
        hair.roughnessR = glm::vec2 { 0.002F, 0.3F };
        hair.tintTT = glm::vec3 { 0.0F };
        hair.tintTRT = glm::vec3 { 0.0F };
        hair.cuticleAngle = angle;
        const scattering::Fibre fibre = fibreOf(hair, 0.0F);
        const float thetaO = 0.4F;
        const glm::vec3 outgoing = fibreward(std::sin(thetaO), 1.2F);
        float best = 0.0F;
        float peak = 0.0F;
        for (int i = 0; i <= 4000; ++i) {
            const float thetaI = -1.5F + (3.0F * static_cast<float>(i) / 4000.0F);
            const glm::vec3 incoming = fibreward(std::sin(thetaI), 1.2F);
            const float value = scattering::Hair::evaluate(fibre, outgoing, incoming)[0] * std::fabs(incoming.z);
            if (value > best) {
                best = value;
                peak = thetaI;
            }
        }
        const float tilt = (angle * 3.14159265F) - (0.5F * 3.14159265F);
        BOOST_TEST(std::fabs(peak - (-thetaO - (2.0F * tilt))) < 2.0e-3F, angle << ": " << peak);
    }
}

// a fibre of MaterialX's parameters holds each roughness to [0.001, 1], scales each azimuthal
// roughness by the root of pi over eight, and gives the last lobe TRT's variance
BOOST_AUTO_TEST_CASE(moya_hair_a_fibre_holds_materialxs_parameters_test) {
    kernel::surface::Hair hair;
    hair.roughnessR = glm::vec2 { 0.0F, 2.0F };
    hair.roughnessTT = glm::vec2 { 0.3F, 0.4F };
    hair.roughnessTRT = glm::vec2 { 0.6F, 0.7F };
    hair.ior = 1.7F;
    const scattering::Fibre fibre = fibreOf(hair, 1.5F);
    BOOST_TEST(fibre.variance[0] == 0.001F);
    BOOST_TEST(fibre.scale[0] == scattering::kLogisticScale);
    BOOST_TEST(fibre.variance[1] == 0.3F);
    BOOST_TEST(fibre.scale[1] == 0.4F * scattering::kLogisticScale);
    BOOST_TEST(fibre.variance[2] == 0.6F);
    BOOST_TEST(fibre.variance[3] == 0.6F);
    BOOST_TEST(fibre.scale[2] == 0.7F * scattering::kLogisticScale);
    BOOST_TEST(fibre.ior == 1.7F);
    BOOST_TEST(fibre.offset == 1.0F);
    for (const float sine : fibre.tiltSine) {
        BOOST_TEST(sine == 0.0F);
    }
}

/** A MaterialX node of a definition. **/
scene::Handle node(scene::Scene & scene, const std::string & name, const char * definition) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    return out;
}

/** Sets a node's input, declaring it of the value's type. **/
void setInput(scene::Scene & scene, const scene::Handle & node, const std::string & name, const scene::Value & value) {
    scene.declare(node, scene::Declaration { .name = name, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
    scene.set(node, name, value);
}

/** Connects an output of one node to an input of another. **/
void connect(scene::Scene & scene, const scene::Handle & from, const std::string & output, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", input + " " + std::to_string(to.index));
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "output", scene::Value::string(output));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** A material whose surface is a surface node over a BSDF node, which it gives back. **/
scene::Handle hairMaterial(scene::Scene & scene, const std::string & name, const char * definition, scene::Handle & bsdfNode) {
    const scene::Handle surface = node(scene, name + " surface", "ND_surface");
    bsdfNode = node(scene, name + " bsdf", definition);
    connect(scene, bsdfNode, "out", surface, "bsdf");
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "surface", scene::Value::handle(surface));
    return out;
}

material::Compiled compileOf(const scene::Scene & scene, const scene::Handle & held) {
    std::vector<std::string> primvars { "N", "Cs", "tangent", "st" };
    return material::compile(material::Library::installed(), scene, held, primvars);
}

/** How many of the reports mention a text. **/
std::size_t mentioning(const std::vector<std::string> & reports, const std::string & text) {
    return static_cast<std::size_t>(std::count_if(reports.begin(), reports.end(), [&](const std::string & report) {
        return report.find(text) != std::string::npos;
    }));
}

// a surface node over chiang_hair_bsdf compiles to a hair, each input the value it is given, and
// each helper node flattened into the formulas of MaterialX's own implementation
BOOST_AUTO_TEST_CASE(moya_hair_a_materialx_hair_compiles_test) {
    scene::Scene scene(scene::kinds());
    scene::Handle hair;
    const scene::Handle held = hairMaterial(scene, "hair", "ND_chiang_hair_bsdf", hair);
    setInput(scene, hair, "ior", scene::Value::number(1.6F));
    setInput(scene, hair, "tint_TT", scene::Value::colour(glm::vec3 { 0.9F, 0.7F, 0.5F }));
    setInput(scene, hair, "cuticle_angle", scene::Value::number(0.45F));
    const scene::Handle roughness = node(scene, "roughness", "ND_chiang_hair_roughness");
    setInput(scene, roughness, "longitudinal", scene::Value::number(0.3F));
    setInput(scene, roughness, "azimuthal", scene::Value::number(0.4F));
    for (const std::string lobe : { "R", "TT", "TRT" }) {
        connect(scene, roughness, "roughness_" + lobe, hair, "roughness_" + lobe);
    }
    const scene::Handle absorption = node(scene, "fromColour", "ND_chiang_hair_absorption_from_color");
    setInput(scene, absorption, "color", scene::Value::colour(glm::vec3 { 0.3F, 0.2F, 0.1F }));
    setInput(scene, absorption, "azimuthal_roughness", scene::Value::number(0.4F));
    connect(scene, absorption, "absorption", hair, "absorption_coefficient");
    scene.commit();
    scene.apply();

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.reports.empty(), (compiled.reports.empty() ? std::string() : compiled.reports.front()));
    const kernel::surface::Material & made = compiled.material;
    BOOST_TEST(made.haired == 1U);
    BOOST_TEST(made.layered == 0U);
    BOOST_TEST(made.hair.ior == 1.6F);
    BOOST_TEST(made.hair.cuticleAngle == 0.45F);
    BOOST_TEST(made.hair.tintTT.y == 0.7F);
    BOOST_TEST(made.hair.tintR.x == 1.0F);
    const float variance = varianceOf(0.3F);
    const float azimuthal = azimuthalOf(0.4F);
    BOOST_TEST(made.hair.roughnessR.x == variance, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(made.hair.roughnessR.y == azimuthal, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(made.hair.roughnessTT.x == 0.25F * variance, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(made.hair.roughnessTRT.x == 4.0F * variance, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(made.hair.roughnessTRT.y == azimuthal, boost::test_tools::tolerance(1.0e-5F));
    const float betaN = 0.4F;
    const float factor = 5.969F - (0.215F * betaN) + (2.532F * betaN * betaN) - (10.73F * betaN * betaN * betaN) + (5.574F * betaN * betaN * betaN * betaN) +
        (0.245F * betaN * betaN * betaN * betaN * betaN);
    const glm::vec3 colour { 0.3F, 0.2F, 0.1F };
    for (int channel = 0; channel < 3; ++channel) {
        const float root = std::log(colour[channel]) / factor;
        BOOST_TEST(made.hair.absorption[channel] == root * root, boost::test_tools::tolerance(1.0e-4F));
    }
}

// melanin gives a hair's absorption as d'Eon and others' fit does, through MaterialX's defaults of
// each melanin's colour, which are in linear Rec. 709
BOOST_AUTO_TEST_CASE(moya_hair_melanin_gives_its_absorption_test) {
    scene::Scene scene(scene::kinds());
    scene::Handle hair;
    const scene::Handle held = hairMaterial(scene, "hair", "ND_chiang_hair_bsdf", hair);
    const scene::Handle melanin = node(scene, "pigment", "ND_deon_hair_absorption_from_melanin");
    setInput(scene, melanin, "melanin_concentration", scene::Value::number(0.6F));
    setInput(scene, melanin, "melanin_redness", scene::Value::number(0.2F));
    connect(scene, melanin, "absorption", hair, "absorption_coefficient");
    scene.commit();
    scene.apply();

    const material::Compiled compiled = compileOf(scene, held);
    BOOST_TEST(compiled.reports.empty(), (compiled.reports.empty() ? std::string() : compiled.reports.front()));
    const float amount = -std::log(1.0F - 0.6F);
    const glm::vec3 eumelanin { 0.657704F, 0.498077F, 0.254107F };
    const glm::vec3 pheomelanin { 0.829444F, 0.67032F, 0.349938F };
    for (int channel = 0; channel < 3; ++channel) {
        const float want = (amount * 0.8F * -std::log(eumelanin[channel])) + (amount * 0.2F * -std::log(pheomelanin[channel]));
        BOOST_TEST(compiled.material.hair.absorption[channel] == want, boost::test_tools::tolerance(1.0e-4F));
    }
    // each melanin's colour is read a component at a time, so each of the two is three constants
    BOOST_TEST(std::ranges::count(compiled.spaces, std::string("lin_rec709")) == 6);
}

// a surface node over a BSDF other than a hair's is reported and takes OpenPBR's defaults, and a
// hair's surface that emits, or a hair given its direction, is reported
BOOST_AUTO_TEST_CASE(moya_hair_what_a_hair_does_not_take_is_reported_test) {
    scene::Scene scene(scene::kinds());
    scene::Handle diffuse;
    const scene::Handle other = hairMaterial(scene, "matte", "ND_oren_nayar_diffuse_bsdf", diffuse);
    scene::Handle hair;
    const scene::Handle held = hairMaterial(scene, "hair", "ND_chiang_hair_bsdf", hair);
    const scene::Handle direction = node(scene, "direction", "ND_constant_vector3");
    connect(scene, direction, "out", hair, "curve_direction");
    scene.commit();
    scene.apply();

    const material::Compiled matte = compileOf(scene, other);
    BOOST_TEST(matte.material.haired == 0U);
    BOOST_TEST(matte.material.layered == 1U);
    BOOST_TEST(mentioning(matte.reports, "over ND_oren_nayar_diffuse_bsdf") == 1U);
    const material::Compiled given = compileOf(scene, held);
    BOOST_TEST(given.material.haired == 1U);
    BOOST_TEST(mentioning(given.reports, "is given its curve_direction") == 1U);
}

/** An instance, in the world, of a prototype with a material. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the prototype, then the material
void placed(scene::Scene & scene, const std::string & name, const scene::Handle & prototype, const scene::Handle & held, const glm::mat4 & transform) {
    const scene::Handle instance = instanced(scene, name, prototype, {}, transform);
    scene.set(instance, "material", scene::Value::handle(held));
}

/** A straight round curve along x, a unit wide, five units up. **/
scene::Handle straight(scene::Scene & scene) {
    const scene::Handle out = scene.create("curves", "hair");
    scene.set(out, "P", scene::Value::points({ { -2.0F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F } }));
    scene.set(out, "vertices", scene::Value::integers({ 2 }));
    scene.set(out, "type", scene::Value::string("linear"));
    scene.set(out, "profile", scene::Value::string("round"));
    scene.set(out, "constantWidth", scene::Value::number(1.0F));
    return out;
}

// a hair's material on a curve shades by its hair, along the fibre. On a round curve it meets the
// fibre at the offset MaterialX takes from the normal, which is minus the hit's place across. That
// offset is the sine of the angle about the fibre between the normal and the direction back along
// the ray, negative where that direction leans towards the bitangent. On what is not a curve the
// material is reported, once
BOOST_AUTO_TEST_CASE(moya_hair_shades_a_curve_test) {
    scene::Scene scene(scene::kinds());
    scene::Handle hair;
    const scene::Handle held = hairMaterial(scene, "hair", "ND_chiang_hair_bsdf", hair);
    placed(scene, "fibre", straight(scene), held, moved(0.0F, 0.0F, 5.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes curved = geometry::gather(scene, {});
    BOOST_TEST(!reported(curved, "is a hair's"));

    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 0.3F, 0.2F, 0.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
    CpuDevice device;
    const view::Traced traced = view::trace(device, curved, { ray }, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, curved, v3d::moya::colour::Config());
    const kernel::surface::Surfaces view = surfaces.view();
    const kernel::surface::ShadingPoint point = view.at(traced.nearest.front(), ray);
    BOOST_TEST_REQUIRE(point.found());
    const bsdf::Bsdf shaded =
        bsdf::atSurface(view, point, colour::acescgTable(), v3d::moya::material::ggxAlbedos(), spectrum::SampledWavelengths::visible(0.5F), -ray.direction);
    BOOST_TEST_REQUIRE(shaded.count == 1U);
    BOOST_TEST((shaded.closures[0].model == bsdf::Model::Hair));
    BOOST_TEST(shaded.fibre.offset == -point.across);
    BOOST_TEST(std::fabs(point.across) > 0.3F);
    const glm::vec3 back = -ray.direction;
    const glm::vec3 around = glm::normalize(back - (shaded.frame.tangent * glm::dot(back, shaded.frame.tangent)));
    const float cosine = glm::dot(shaded.frame.normal, around);
    const float sine = std::sqrt(std::fmax(0.0F, 1.0F - (cosine * cosine)));
    BOOST_TEST(std::fabs(shaded.fibre.offset - (glm::dot(around, shaded.frame.bitangent) > 0.0F ? -sine : sine)) < 1.0e-2F);
    BOOST_TEST(std::fabs(glm::dot(shaded.frame.tangent, glm::vec3 { 1.0F, 0.0F, 0.0F })) > 1.0F - 1.0e-5F);

    const scene::Handle ball = scene.create("sphere", "ball");
    placed(scene, "ball", ball, held, moved(0.0F, 4.0F, 5.0F));
    placed(scene, "another ball", ball, held, moved(0.0F, 8.0F, 5.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes mixed = geometry::gather(scene, {});
    BOOST_TEST(mentioning(mixed.reports, "material hair is a hair's") == 1U);
}

};  // namespace

};  // namespace v3d::moya::tracing
