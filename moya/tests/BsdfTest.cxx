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
#include <moya/libmoya/kernel/bsdf/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
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

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr float kPi = 3.14159265F;

/** A diffuse BSDF of a constant reflectance, shaded and lying about a normal. **/
bsdf::Bsdf diffuseAbout(const glm::vec3 & shading, const glm::vec3 & geometric, float reflectance) {
    bsdf::Bsdf out;
    out.model = bsdf::Model::Diffuse;
    out.diffuse.reflectance = kernel::spectrum::SampledSpectrum(reflectance);
    out.frame = kernel::surface::Frame::around(glm::normalize(shading));
    out.geometric = glm::normalize(geometric);
    return out;
}

/** The value of chi-squared that a fit of a number of degrees of freedom passes below. **/
float chiSquaredBound(std::size_t freedom) {
    // the statistic's mean is its freedom and its deviation the root of twice it; six deviations
    // above the mean is a false failure less than once in a billion
    return static_cast<float>(freedom) + (6.0F * std::sqrt(2.0F * static_cast<float>(freedom)));
}

/** A shader node with a constant colour Cs, and a material whose surface it is. **/
scene::Handle colouredMaterial(scene::Scene & scene, const std::string & name, const glm::vec3 & colour) {
    const scene::Handle node = scene.create("shader", name);
    scene.set(node, "shader", scene::Value::string("plastic"));
    scene.set(node, "language", scene::Value::string("ri"));
    scene.declare(node, scene::Declaration { .name = "Cs", .type = scene::Type::Colour, .fallback = scene::Value::colour(glm::vec3 { 1.0F }) });
    scene.set(node, "Cs", scene::Value::colour(colour));
    const scene::Handle material = scene.create("material", name);
    scene.set(material, "surface", scene::Value::handle(node));
    return material;
}

/** A unit square in z = 0 facing +z, as one face, with a constant Cs primvar when given one. **/
scene::Handle square(scene::Scene & scene, const std::string & name) {
    const scene::Handle out = scene.create("mesh", name);
    scene.set(out, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(out, "vertices", scene::Value::integers({ 4 }));
    scene.set(out, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    return out;
}

/** The reflectance the BSDF at the middle of a scene's square holds, looked at from above. **/
kernel::spectrum::SampledSpectrum reflectanceOf(const scene::Scene & scene, const kernel::spectrum::SampledWavelengths & wavelengths) {
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 0.1F, 0.2F, 3.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    CpuDevice device;
    const view::Traced traced = view::trace(device, prototypes, { ray }, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    const kernel::surface::ShadingPoint point = held.at(traced.nearest.front(), ray);
    BOOST_TEST_REQUIRE(point.found());
    return bsdf::atSurface(held, point, colour::acescgTable(), wavelengths).diffuse.reflectance;
}

/** Whether two spectra are equal at each wavelength, within a tolerance. **/
bool near(const kernel::spectrum::SampledSpectrum & left, const kernel::spectrum::SampledSpectrum & right) {
    for (int i = 0; i < kernel::spectrum::kSamples; ++i) {
        if (std::fabs(left[i] - right[i]) > 1.0e-5F) {
            return false;
        }
    }
    return true;
}

// directions drawn from the diffuse lobe fall into bins of the hemisphere as its density says:
// a share of the cosine's square between a bin's bounds, evenly around the normal
BOOST_AUTO_TEST_CASE(moya_bsdf_diffuse_samples_follow_their_density_test) {
    constexpr std::size_t kHeights = 10;
    constexpr std::size_t kTurns = 20;
    constexpr std::size_t kDrawn = 200000;
    const bsdf::Bsdf diffuse = diffuseAbout({ 0.3F, -0.2F, 1.0F }, { 0.3F, -0.2F, 1.0F }, 0.7F);
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { 0.2F, 0.5F, 1.0F });
    std::vector<std::size_t> counts(kHeights * kTurns, 0);
    std::uint32_t seed = 5;
    for (std::size_t i = 0; i < kDrawn; ++i) {
        const glm::vec2 u { next(seed), next(seed) };
        const bsdf::Sample drawn = diffuse.sample(outgoing, u);
        BOOST_TEST_REQUIRE(drawn.found());
        BOOST_TEST_REQUIRE((drawn.lobe == bsdf::Lobe::Diffuse));
        const glm::vec3 local = diffuse.frame.toLocal(drawn.incoming);
        BOOST_TEST_REQUIRE(drawn.density == diffuse.density(outgoing, drawn.incoming), boost::test_tools::tolerance(1.0e-4F));
        BOOST_TEST_REQUIRE(drawn.density == local.z / kPi, boost::test_tools::tolerance(1.0e-3F));
        const auto height = std::min(static_cast<std::size_t>(local.z * kHeights), kHeights - 1);
        const float turn = (std::atan2(local.y, local.x) + kPi) / (2.0F * kPi);
        const auto around = std::min(static_cast<std::size_t>(turn * kTurns), kTurns - 1);
        ++counts[(height * kTurns) + around];
    }
    float chiSquared = 0.0F;
    for (std::size_t height = 0; height < kHeights; ++height) {
        const float low = static_cast<float>(height) / kHeights;
        const float high = static_cast<float>(height + 1) / kHeights;
        const float expected = static_cast<float>(kDrawn) * ((high * high) - (low * low)) / static_cast<float>(kTurns);
        for (std::size_t around = 0; around < kTurns; ++around) {
            const float difference = static_cast<float>(counts[(height * kTurns) + around]) - expected;
            chiSquared += difference * difference / expected;
        }
    }
    BOOST_TEST(chiSquared < chiSquaredBound((kHeights * kTurns) - 1));
}

// the diffuse lobe reflects as much light as its reflectance says: its value times the cosine,
// integrated over the hemisphere, is the reflectance, and each sample's weight is it exactly
BOOST_AUTO_TEST_CASE(moya_bsdf_diffuse_reflects_its_reflectance_test) {
    const bsdf::Bsdf diffuse = diffuseAbout({ 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F, 1.0F }, 0.8F);
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { 0.4F, 0.0F, 1.0F });
    constexpr int kSide = 400;
    double integral = 0.0;
    for (int row = 0; row < kSide; ++row) {
        for (int column = 0; column < kSide; ++column) {
            // a grid of the hemisphere, even in the cosine and the turn, each cell of equal area
            const float z = (static_cast<float>(row) + 0.5F) / kSide;
            const float turn = 2.0F * kPi * (static_cast<float>(column) + 0.5F) / kSide;
            const float across = std::sqrt(1.0F - (z * z));
            const glm::vec3 incoming { across * std::cos(turn), across * std::sin(turn), z };
            integral += static_cast<double>(diffuse.evaluate(outgoing, incoming)[0]) * z * (2.0 * kPi / (kSide * kSide));
        }
    }
    BOOST_TEST(integral == 0.8, boost::test_tools::tolerance(1.0e-3));

    std::uint32_t seed = 9;
    for (int i = 0; i < 1000; ++i) {
        const bsdf::Sample drawn = diffuse.sample(outgoing, { next(seed), next(seed) });
        BOOST_TEST_REQUIRE(drawn.found());
        BOOST_TEST(drawn.value[0] * glm::dot(drawn.incoming, diffuse.geometric) / drawn.density == 0.8F, boost::test_tools::tolerance(1.0e-4F));
    }
}

// with the shading normal tilted from the geometric one, a sample or a direction that would
// cross the geometric surface has no value and no density
BOOST_AUTO_TEST_CASE(moya_bsdf_a_reflection_below_the_geometric_surface_is_refused_test) {
    const glm::vec3 geometric { 0.0F, 0.0F, 1.0F };
    const bsdf::Bsdf tilted = diffuseAbout({ 1.0F, 0.0F, 0.6F }, geometric, 0.5F);
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { 1.0F, 0.0F, 0.3F });
    std::uint32_t seed = 13;
    int refused = 0;
    int kept = 0;
    for (int i = 0; i < 5000; ++i) {
        const bsdf::Sample drawn = tilted.sample(outgoing, { next(seed), next(seed) });
        if (glm::dot(drawn.incoming, geometric) <= 0.0F) {
            ++refused;
            BOOST_TEST(!drawn.found());
            BOOST_TEST(drawn.value[0] == 0.0F);
        } else {
            ++kept;
            BOOST_TEST(drawn.found());
        }
    }
    BOOST_TEST(refused > 100);
    BOOST_TEST(kept > 100);
    const glm::vec3 below = glm::normalize(glm::vec3 { 1.0F, 0.0F, -0.1F });
    BOOST_TEST(tilted.evaluate(outgoing, below)[0] == 0.0F);
    BOOST_TEST(tilted.density(outgoing, below) == 0.0F);
}

// the diffuse lobe reflects on whichever side light leaves towards, and never through the surface
BOOST_AUTO_TEST_CASE(moya_bsdf_diffuse_reflects_on_either_side_test) {
    const bsdf::Bsdf diffuse = diffuseAbout({ 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F, 1.0F }, 0.5F);
    const glm::vec3 under = glm::normalize(glm::vec3 { 0.3F, 0.1F, -1.0F });
    std::uint32_t seed = 17;
    for (int i = 0; i < 200; ++i) {
        const bsdf::Sample drawn = diffuse.sample(under, { next(seed), next(seed) });
        BOOST_TEST_REQUIRE(drawn.found());
        BOOST_TEST(drawn.incoming.z < 0.0F);
    }
    BOOST_TEST(diffuse.evaluate(under, { 0.0F, 0.0F, 1.0F })[0] == 0.0F);
    BOOST_TEST(diffuse.evaluate(under, { 0.0F, 0.0F, -1.0F })[0] == 0.5F / kPi, boost::test_tools::tolerance(1.0e-5F));
}

// a surface's colour is its Cs primvar where it has one, its material's surface node's Cs where
// it has not, and a mid grey with neither
BOOST_AUTO_TEST_CASE(moya_bsdf_a_surfaces_colour_comes_from_its_primvar_then_its_material_test) {
    const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(0.37F);
    const kernel::spectrum::RgbTable table = colour::acescgTable();
    const glm::vec3 primvar { 0.9F, 0.2F, 0.1F };
    const glm::vec3 node { 0.1F, 0.3F, 0.8F };
    for (const int given : { 0, 1, 2 }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle face = square(scene, "square");
        if (given == 2) {
            scene.declare(face, scene::Declaration { .name = "Cs",
                .type = scene::Type::Colour,
                .count = scene::Declaration::kAny,
                .fallback = scene::Value::colours({}),
                .interpolation = scene::Interpolation::Constant });
            scene.set(face, "Cs", scene::Value::colours({ primvar }));
        }
        const scene::Handle placed = instanced(scene, "square", face, {}, glm::mat4(1.0F));
        if (given >= 1) {
            scene.set(placed, "material", scene::Value::handle(colouredMaterial(scene, "blue", node)));
        }
        scene.commit();
        scene.apply();
        glm::vec3 expected { 0.5F };
        if (given == 2) {
            expected = primvar;
        } else if (given == 1) {
            expected = node;
        }
        BOOST_TEST_CONTEXT("given " << given) {
            BOOST_TEST(near(reflectanceOf(scene, wavelengths), kernel::spectrum::reflectance(table, expected, wavelengths)));
        }
    }
}

// a square in a group, placed by an instance with a material, takes that material; given one of
// its own, it keeps its own
BOOST_AUTO_TEST_CASE(moya_bsdf_a_group_member_takes_the_material_that_places_it_test) {
    const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(0.61F);
    const kernel::spectrum::RgbTable table = colour::acescgTable();
    const glm::vec3 outer { 0.2F, 0.7F, 0.3F };
    const glm::vec3 inner { 0.8F, 0.6F, 0.1F };
    for (const bool own : { false, true }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle group = scene.create("group", "group");
        const scene::Handle member = instanced(scene, "member", square(scene, "square"), group, glm::mat4(1.0F));
        if (own) {
            scene.set(member, "material", scene::Value::handle(colouredMaterial(scene, "inner", inner)));
        }
        const scene::Handle placing = instanced(scene, "placing", group, {}, glm::mat4(1.0F));
        scene.set(placing, "material", scene::Value::handle(colouredMaterial(scene, "outer", outer)));
        scene.commit();
        scene.apply();
        BOOST_TEST_CONTEXT((own ? "its own" : "none of its own")) {
            BOOST_TEST(near(reflectanceOf(scene, wavelengths), kernel::spectrum::reflectance(table, own ? inner : outer, wavelengths)));
        }
    }
}

};  // namespace

};  // namespace v3d::moya::tracing
