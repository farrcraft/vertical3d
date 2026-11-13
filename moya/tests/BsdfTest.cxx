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
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

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
namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr float kPi = 3.14159265F;

/** A BSDF of no closures, shaded and lying about a normal. **/
bsdf::Bsdf emptyAbout(const glm::vec3 & shading, const glm::vec3 & geometric) {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::normalize(shading));
    out.coatFrame = out.frame;
    out.geometric = glm::normalize(geometric);
    return out;
}

/** A diffuse BSDF of a constant reflectance, shaded and lying about a normal. **/
bsdf::Bsdf diffuseAbout(const glm::vec3 & shading, const glm::vec3 & geometric, float reflectance) {
    bsdf::Bsdf out = emptyAbout(shading, geometric);
    out.add(bsdf::Model::Diffuse, kernel::spectrum::SampledSpectrum(reflectance));
    return out;
}

/** A spectrum of one value at every wavelength. **/
kernel::spectrum::SampledSpectrum flat(float value) {
    return kernel::spectrum::SampledSpectrum(value);
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
    const bsdf::Bsdf scattering = bsdf::atSurface(held, point, colour::acescgTable(), v3d::moya::material::ggxAlbedos(), wavelengths, glm::vec3 { 0.0F, 0.0F, 1.0F });
    BOOST_TEST_REQUIRE(scattering.count == 1U);
    BOOST_TEST_REQUIRE((scattering.closures[0].model == bsdf::Model::Diffuse));
    return scattering.closures[0].weight;
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

// directions drawn from the diffuse lobe fall into bins of the sphere as its density says, and
// each density is its cosine over pi
BOOST_AUTO_TEST_CASE(moya_bsdf_diffuse_samples_follow_their_density_test) {
    const bsdf::Bsdf diffuse = diffuseAbout({ 0.3F, -0.2F, 1.0F }, { 0.3F, -0.2F, 1.0F }, 0.7F);
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { 0.2F, 0.5F, 1.0F });
    const Fit fit = densityFit(diffuse, outgoing, 200000, 5);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
    std::uint32_t seed = 7;
    for (int i = 0; i < 1000; ++i) {
        const bsdf::Sample drawn = diffuse.sample(outgoing, { next(seed), next(seed) }, 0.5F);
        BOOST_TEST_REQUIRE(drawn.found());
        BOOST_TEST_REQUIRE((drawn.lobe == bsdf::Lobe::Diffuse));
        BOOST_TEST_REQUIRE(!drawn.specular);
        BOOST_TEST_REQUIRE(drawn.density == diffuse.frame.toLocal(drawn.incoming).z / kPi, boost::test_tools::tolerance(1.0e-3F));
    }
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
        const bsdf::Sample drawn = diffuse.sample(outgoing, { next(seed), next(seed) }, 0.5F);
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
        const bsdf::Sample drawn = tilted.sample(outgoing, { next(seed), next(seed) }, 0.5F);
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
        const bsdf::Sample drawn = diffuse.sample(under, { next(seed), next(seed) }, 0.5F);
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

// two diffuse closures of half a reflectance each scatter as one of the whole: the same value and
// density in every direction, and samples that follow that density
BOOST_AUTO_TEST_CASE(moya_bsdf_two_halves_of_a_diffuse_lobe_scatter_as_one_test) {
    const glm::vec3 normal { 0.1F, 0.2F, 1.0F };
    const bsdf::Bsdf whole = diffuseAbout(normal, normal, 0.6F);
    bsdf::Bsdf halves = emptyAbout(normal, normal);
    halves.add(bsdf::Model::Diffuse, flat(0.3F));
    halves.add(bsdf::Model::Diffuse, flat(0.3F));
    BOOST_TEST_REQUIRE(halves.count == 2U);
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { -0.3F, 0.4F, 1.0F });
    std::uint32_t seed = 21;
    for (int i = 0; i < 200; ++i) {
        const glm::vec3 incoming = glm::normalize(glm::vec3 { next(seed) - 0.5F, next(seed) - 0.5F, next(seed) });
        BOOST_TEST(halves.evaluate(outgoing, incoming)[0] == whole.evaluate(outgoing, incoming)[0], boost::test_tools::tolerance(1.0e-5F));
        BOOST_TEST(halves.density(outgoing, incoming) == whole.density(outgoing, incoming), boost::test_tools::tolerance(1.0e-5F));
    }
    const Fit fit = densityFit(halves, outgoing, 200000, 23);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
    BOOST_TEST(furnace(halves, outgoing, 20000, 25) == 0.6F, boost::test_tools::tolerance(1.0e-4F));
}

// a coated closure works in the coat's frame, and the projected value takes each closure's value
// times the cosine to its own frame's normal, as its samples are weighed
BOOST_AUTO_TEST_CASE(moya_bsdf_a_coated_closure_works_in_the_coats_frame_test) {
    const glm::vec3 up { 0.0F, 0.0F, 1.0F };
    const glm::vec3 tilted = glm::normalize(glm::vec3 { 0.4F, -0.2F, 1.0F });
    bsdf::Bsdf layered = emptyAbout(up, up);
    layered.coatFrame = kernel::surface::Frame::around(tilted);
    layered.add(bsdf::Model::Diffuse, flat(0.4F));
    bsdf::Closure coat;
    coat.model = bsdf::Model::Diffuse;
    coat.weight = flat(0.5F);
    coat.coated = true;
    layered.add(coat);
    BOOST_TEST_REQUIRE(layered.count == 2U);
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { -0.3F, 0.4F, 1.0F });
    std::uint32_t seed = 27;
    for (int i = 0; i < 200; ++i) {
        const glm::vec3 incoming = glm::normalize(glm::vec3 { next(seed) - 0.5F, next(seed) - 0.5F, next(seed) + 0.2F });
        const float onUp = glm::dot(incoming, up);
        const float onTilted = glm::dot(incoming, tilted);
        const float below = onUp > 0.0F ? 0.4F / kPi : 0.0F;
        const float coated = onTilted > 0.0F ? 0.5F / kPi : 0.0F;
        BOOST_TEST(layered.evaluate(outgoing, incoming)[0] == below + coated, boost::test_tools::tolerance(1.0e-5F));
        BOOST_TEST(layered.projected(outgoing, incoming)[0] == (below * onUp) + (coated * onTilted), boost::test_tools::tolerance(1.0e-5F));
        const float density = ((onUp > 0.0F ? 0.4F * onUp : 0.0F) + (onTilted > 0.0F ? 0.5F * onTilted : 0.0F)) / (0.9F * kPi);
        BOOST_TEST(layered.density(outgoing, incoming) == density, boost::test_tools::tolerance(1.0e-5F));
    }
    const Fit fit = densityFit(layered, outgoing, 200000, 29);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
}

// a mirror sends light from one direction alone, the reflection of outgoing about the shading
// normal. Its sample is specular, of its weight and the chance of its closure, and every
// direction it is given has no value and no density
BOOST_AUTO_TEST_CASE(moya_bsdf_a_mirror_reflects_about_the_shading_normal_test) {
    const glm::vec3 normal = glm::normalize(glm::vec3 { 0.2F, -0.1F, 1.0F });
    bsdf::Bsdf mirror = emptyAbout(normal, normal);
    mirror.add(bsdf::Model::Mirror, flat(0.9F));
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { 0.5F, 0.3F, 1.0F });
    const glm::vec3 reflected = (2.0F * glm::dot(outgoing, normal) * normal) - outgoing;
    std::uint32_t seed = 31;
    for (int i = 0; i < 100; ++i) {
        const bsdf::Sample drawn = mirror.sample(outgoing, { next(seed), next(seed) }, 0.5F);
        BOOST_TEST_REQUIRE(drawn.found());
        BOOST_TEST(drawn.specular);
        BOOST_TEST((drawn.lobe == bsdf::Lobe::Specular));
        BOOST_TEST(glm::distance(drawn.incoming, reflected) < 1.0e-5F);
        BOOST_TEST(drawn.value[0] == 0.9F);
        BOOST_TEST(drawn.density == 1.0F);
    }
    BOOST_TEST(mirror.evaluate(outgoing, reflected)[0] == 0.0F);
    BOOST_TEST(mirror.density(outgoing, reflected) == 0.0F);
    BOOST_TEST(furnace(mirror, outgoing, 100, 33) == 0.9F, boost::test_tools::tolerance(1.0e-5F));
}

// a mirror whose shading normal is tilted far from the geometric one refuses a reflection that
// would cross the geometric surface
BOOST_AUTO_TEST_CASE(moya_bsdf_a_mirror_refuses_a_reflection_below_the_geometric_surface_test) {
    bsdf::Bsdf mirror = emptyAbout({ 1.0F, 0.0F, 0.6F }, { 0.0F, 0.0F, 1.0F });
    mirror.add(bsdf::Model::Mirror, flat(1.0F));
    const bsdf::Sample grazing = mirror.sample(glm::normalize(glm::vec3 { -1.0F, 0.0F, 0.2F }), { 0.5F, 0.5F }, 0.5F);
    BOOST_TEST(grazing.specular);
    BOOST_TEST(grazing.incoming.z < 0.0F);
    BOOST_TEST(!grazing.found());
    BOOST_TEST(mirror.sample(glm::normalize(glm::vec3 { 1.0F, 0.0F, 0.6F }), { 0.5F, 0.5F }, 0.5F).found());
}

// a diffuse closure beside a mirror: each is picked by its share of the albedo, the diffuse
// samples follow the density the diffuse closure keeps, and a white furnace returns both
BOOST_AUTO_TEST_CASE(moya_bsdf_a_sample_picks_a_closure_by_its_albedo_test) {
    const glm::vec3 normal { 0.0F, 0.0F, 1.0F };
    bsdf::Bsdf both = emptyAbout(normal, normal);
    both.add(bsdf::Model::Diffuse, flat(0.6F));
    both.add(bsdf::Model::Mirror, flat(0.2F));
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { 0.3F, 0.0F, 1.0F });
    constexpr int kDrawn = 40000;
    std::uint32_t seed = 41;
    int specular = 0;
    for (int i = 0; i < kDrawn; ++i) {
        const bsdf::Sample drawn = both.sample(outgoing, { next(seed), next(seed) }, 0.5F);
        BOOST_TEST_REQUIRE(drawn.found());
        if (drawn.specular) {
            ++specular;
            BOOST_TEST_REQUIRE(drawn.density == 0.25F, boost::test_tools::tolerance(1.0e-6F));
        } else {
            BOOST_TEST_REQUIRE(drawn.density == 0.75F * drawn.incoming.z / kPi, boost::test_tools::tolerance(1.0e-4F));
            BOOST_TEST_REQUIRE(drawn.value[0] == 0.6F / kPi, boost::test_tools::tolerance(1.0e-5F));
        }
    }
    // the count of specular samples is binomial, of a deviation near 87 over 40000
    BOOST_TEST(std::fabs(static_cast<float>(specular) - (0.25F * kDrawn)) < 450.0F);
    const Fit fit = densityFit(both, outgoing, 200000, 43);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
    BOOST_TEST(furnace(both, outgoing, 200000, 45) == 0.8F, boost::test_tools::tolerance(1.0e-2F));
}

// a closure of no albedo is left out, and a full BSDF keeps the closures of the largest albedo
BOOST_AUTO_TEST_CASE(moya_bsdf_a_full_bsdf_keeps_its_largest_closures_test) {
    bsdf::Bsdf full = emptyAbout({ 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F, 1.0F });
    full.add(bsdf::Model::Diffuse, flat(0.0F));
    BOOST_TEST(full.count == 0U);
    BOOST_TEST(!full.sample({ 0.0F, 0.0F, 1.0F }, { 0.5F, 0.5F }, 0.5F).found());
    for (std::uint32_t i = 0; i <= bsdf::kMostClosures; ++i) {
        full.add(bsdf::Model::Diffuse, flat(0.01F * static_cast<float>(i + 1U)));
    }
    BOOST_TEST(full.count == bsdf::kMostClosures);
    float least = 1.0F;
    for (std::uint32_t i = 0; i < full.count; ++i) {
        least = std::fmin(least, full.closures[i].albedo);
    }
    BOOST_TEST(least == 0.02F, boost::test_tools::tolerance(1.0e-6F));
    const float before = full.albedo();
    full.add(bsdf::Model::Mirror, flat(0.015F));
    BOOST_TEST(full.albedo() == before);
    full.add(bsdf::Model::Mirror, flat(0.5F));
    BOOST_TEST(full.albedo() == before - 0.02F + 0.5F, boost::test_tools::tolerance(1.0e-5F));
}

};  // namespace

};  // namespace v3d::moya::tracing
