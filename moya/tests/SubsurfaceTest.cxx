/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/ConvertedMaterials.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/volume/openpbr/Interior.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/openpbr/Subsurface.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Scattering.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace film = v3d::moya::film;
namespace geometry = v3d::moya::geometry;
namespace integrate = v3d::moya::integrate;
namespace scene = v3d::moya::scene;
namespace spectrum = v3d::moya::kernel::spectrum;
namespace volume = v3d::moya::kernel::volume;

typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSmall = 8;

/** A walk long enough that no case's walk reaches it. **/
constexpr std::int32_t kEndless = 1 << 20;

/** A direction of a polar cosine about z, turned about it. A negative cosine lies below. **/
glm::vec3 towards(float cosine, float turn) {
    const float across = std::sqrt(1.0F - (cosine * cosine));
    return glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine };
}

/** A BSDF shaded about +z with the albedo tables the build integrated, and no closures. **/
bsdf::Bsdf shaded() {
    bsdf::Bsdf out;
    out.frame = kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.coatFrame = out.frame;
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    return out;
}

/** The BSDF of OpenPBR's parameters for light leaving at a cosine to +z. **/
bsdf::Bsdf layered(const kernel::surface::OpenPbr & surface, float cosine) {
    bsdf::Bsdf out = shaded();
    bsdf::Layering::add(out, surface, colour::acescgTable(), spectrum::SampledWavelengths::visible(0.5F), cosine);
    return out;
}

/** A BSDF of one white Oren-Nayar lobe of a roughness that goes through the surface. **/
bsdf::Bsdf translucent(float roughness) {
    bsdf::Bsdf out = shaded();
    bsdf::Closure closure;
    closure.model = bsdf::Model::OrenNayar;
    closure.weight = spectrum::SampledSpectrum(1.0F);
    closure.roughness = roughness;
    closure.through = true;
    out.add(closure);
    return out;
}

/** A shader node of a definition with inputs. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const std::vector<std::pair<std::string, scene::Value>> & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    for (const auto & [input, value] : inputs) {
        scene.declare(out, scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
        scene.set(out, input, value);
    }
    return out;
}

/** A material of an OpenPBR surface of some inputs. **/
scene::Handle openPbr(scene::Scene & scene, const std::string & name, const std::vector<std::pair<std::string, scene::Value>> & inputs) {
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "surface", scene::Value::handle(node(scene, name, kOpenPbr, inputs)));
    return out;
}

/**
 * A closed sphere of a radius at the origin, placed with a material, turned so a camera on the
 * z axis looks at its equator, away from its poles.
 **/
void ball(scene::Scene & scene, const std::string & name, float radius, const scene::Handle & material) {
    const scene::Handle sphere = scene.create("sphere", name);
    scene.set(sphere, "radius", scene::Value::number(radius));
    scene.set(sphere, "zmin", scene::Value::number(-radius));
    scene.set(sphere, "zmax", scene::Value::number(radius));
    const scene::Handle placed = instanced(scene, name, sphere, {}, glm::rotate(glm::mat4(1.0F), glm::radians(90.0F), glm::vec3 { 1.0F, 0.0F, 0.0F }));
    scene.set(placed, "material", scene::Value::handle(material));
}

/**
 * The picture of a scene that build() fills, seen by an orthographic camera five units back along
 * -z, whose screen is a tenth of a unit across, under a white dome light.
 **/
template <typename F>
film::Snapshot pictured(std::int32_t samples, F && build) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSmall, kSmall }));
    scene.set(options, "samples", scene::Value::integer(samples));
    scene.set(camera, "screen", scene::Value::floats({ -0.05F, 0.05F, -0.05F, 0.05F }));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    scene.create("domeLight", "sky");
    build(scene, options);
    scene.commit();
    scene.apply();
    CpuDevice device;
    return integrate::render(device, scene, {}, v3d::moya::colour::Config());
}

/** The mean of each component of a picture's beauty, over every pixel. **/
glm::vec3 meanColour(const film::Snapshot & snapshot) {
    glm::dvec3 sum { 0.0 };
    for (std::uint32_t y = 0; y < snapshot.resolution.y; ++y) {
        for (std::uint32_t x = 0; x < snapshot.resolution.x; ++x) {
            sum += glm::dvec3 { snapshot.at(x, y, 0), snapshot.at(x, y, 1), snapshot.at(x, y, 2) };
        }
    }
    return glm::vec3(sum / static_cast<double>(snapshot.resolution.x * snapshot.resolution.y));
}

/** Whether each component of one colour lies within a share of the other's. **/
bool near(const glm::vec3 & given, const glm::vec3 & expected, float share) {
    for (glm::length_t i = 0; i < 3; ++i) {
        if (!(std::fabs(given[i] - expected[i]) <= share * std::fabs(expected[i]))) {
            return false;
        }
    }
    return true;
}

// OpenPBR's mapping takes a colour of none to an albedo of none and a colour of one to one, and
// grey of a half to 0.9117 when the medium scatters evenly. Scattering forwards needs a higher
// albedo to be seen as the same colour
BOOST_AUTO_TEST_CASE(moya_subsurface_the_colour_maps_to_a_single_scattering_albedo_test) {
    BOOST_TEST(volume::openpbr::singleAlbedo(0.0F, 0.0F) == 0.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(volume::openpbr::singleAlbedo(1.0F, 0.0F) == 1.0F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(volume::openpbr::singleAlbedo(0.5F, 0.0F) == 0.911709F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(volume::openpbr::singleAlbedo(0.5F, 0.6F) > volume::openpbr::singleAlbedo(0.5F, 0.0F));
    BOOST_TEST(volume::openpbr::singleAlbedo(0.5F, -0.6F) < volume::openpbr::singleAlbedo(0.5F, 0.0F));
}

// the medium below a surface takes the radius times its scale as its mean free path in each
// channel, and scatters the share of its extinction the mapping gives, by its anisotropy. A thin
// wall, or a weight of none, holds no medium
BOOST_AUTO_TEST_CASE(moya_subsurface_the_radius_is_the_mean_free_path_test) {
    kernel::surface::OpenPbr surface;
    surface.subsurfaceWeight = 1.0F;
    surface.subsurfaceColour = glm::vec3 { 0.5F, 0.0F, 1.0F };
    surface.subsurfaceRadius = 2.0F;
    surface.subsurfaceRadiusScale = glm::vec3 { 1.0F, 0.5F, 0.25F };
    surface.subsurfaceScatterAnisotropy = 0.3F;
    const volume::Medium medium = volume::openpbr::walked(surface);
    const glm::vec3 extinction = volume::extinction(medium);
    BOOST_TEST(extinction.x == 0.5F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(extinction.y == 1.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(extinction.z == 2.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(medium.lobes[0].scattering.x == 0.5F * volume::openpbr::singleAlbedo(0.5F, 0.3F), boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(medium.lobes[0].scattering.y == 0.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(medium.absorption.z == 0.0F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(medium.lobes[0].anisotropy == 0.3F);
    surface.thinWalled = 1U;
    BOOST_TEST(volume::empty(volume::openpbr::walked(surface)));
    surface.thinWalled = 0U;
    surface.subsurfaceWeight = 0.0F;
    BOOST_TEST(volume::empty(volume::openpbr::walked(surface)));
}

// a material encloses the medium its subsurface gives when it is not thin-walled, as one that
// transmits does
BOOST_AUTO_TEST_CASE(moya_subsurface_a_material_that_subsurfaces_encloses_a_medium_test) {
    kernel::surface::Material material;
    material.layered = 1U;
    BOOST_TEST(!material.encloses());
    material.subsurfaces = 1U;
    BOOST_TEST(material.encloses());
    material.surface.thinWalled = 1U;
    BOOST_TEST(!material.encloses());
}

// the subsurface's share of the opaque base is a dielectric that walks, in place of the diffuse
// base and the specular reflection, and a denoiser sees it as the subsurface colour
BOOST_AUTO_TEST_CASE(moya_subsurface_the_subsurface_is_a_dielectric_that_walks_test) {
    kernel::surface::OpenPbr surface;
    surface.subsurfaceWeight = 1.0F;
    surface.subsurfaceColour = glm::vec3 { 0.6F };
    surface.specularRoughness = 0.0F;
    const bsdf::Bsdf scattering = layered(surface, 0.7F);
    std::uint32_t walks = 0;
    for (std::uint32_t i = 0; i < scattering.count; ++i) {
        const bsdf::Closure & closure = scattering.closures[i];
        BOOST_TEST((closure.model == bsdf::Model::Dielectric) == closure.walks);
        walks += closure.walks ? 1U : 0U;
    }
    BOOST_TEST(walks == 1U);
    BOOST_TEST(scattering.singularShare() == 0.0F);
    const spectrum::SampledSpectrum seen = scattering.spreadAlbedo();
    const spectrum::SampledSpectrum expected =
        spectrum::reflectance(colour::acescgTable(), surface.subsurfaceColour, spectrum::SampledWavelengths::visible(0.5F));
    for (int i = 0; i < spectrum::kSamples; ++i) {
        BOOST_TEST(seen[i] == expected[i], boost::test_tools::tolerance(1.0e-4F));
    }
}

// on a thin wall the subsurface is OpenPBR's sheet: an Oren-Nayar reflection of half of one less
// its anisotropy and a transmission of half of one plus it, with no walk
BOOST_AUTO_TEST_CASE(moya_subsurface_a_thin_wall_reflects_and_transmits_by_its_anisotropy_test) {
    kernel::surface::OpenPbr surface;
    surface.subsurfaceWeight = 1.0F;
    surface.subsurfaceScatterAnisotropy = 0.4F;
    surface.thinWalled = 1U;
    surface.specularWeight = 0.0F;
    const bsdf::Bsdf scattering = layered(surface, 0.7F);
    float reflected = 0.0F;
    float transmitted = 0.0F;
    for (std::uint32_t i = 0; i < scattering.count; ++i) {
        const bsdf::Closure & closure = scattering.closures[i];
        BOOST_TEST(!closure.walks);
        if (closure.model == bsdf::Model::OrenNayar) {
            (closure.through ? transmitted : reflected) += closure.weight[0];
        }
    }
    BOOST_TEST(reflected == 0.3F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(transmitted == 0.7F, boost::test_tools::tolerance(1.0e-4F));
}

// a white surface keeps a furnace's energy whatever share its subsurface takes, thin-walled or
// not. What the walk's dielectric and a thin wall's sheet carry through the surface is counted
// by its energy, so the walk's radiance is scaled by the index squared
BOOST_AUTO_TEST_CASE(moya_subsurface_a_white_surface_keeps_its_energy_at_any_weight_test) {
    std::uint32_t seed = 1551;
    for (const bool thin : { false, true }) {
        for (const float weight : { 0.3F, 0.7F }) {
            BOOST_TEST_CONTEXT((thin ? "thin, weight " : "weight ") << weight) {
                kernel::surface::OpenPbr surface;
                surface.baseColour = glm::vec3 { 1.0F };
                surface.subsurfaceColour = glm::vec3 { 1.0F };
                surface.subsurfaceWeight = weight;
                surface.thinWalled = thin ? 1U : 0U;
                const glm::vec3 outgoing = towards(0.6F, 0.5F);
                // the walk's light crosses into OpenPBR's default index, and a sheet's into air
                const float crossing = thin ? 1.0F : surface.specularIor * surface.specularIor;
                BOOST_TEST(furnace(layered(surface, outgoing.z), outgoing, 200000, seed++, crossing) == 1.0F, boost::test_tools::tolerance(0.02F));
            }
        }
    }
}

// an Oren-Nayar lobe that goes through the surface samples by its density on the far side, and a
// white one sends all a furnace's light through
BOOST_AUTO_TEST_CASE(moya_subsurface_a_diffuse_transmission_follows_its_density_test) {
    std::uint32_t seed = 1501;
    for (const float roughness : { 0.0F, 0.7F }) {
        for (const float cosine : { 0.9F, 0.3F, -0.6F }) {
            BOOST_TEST_CONTEXT("roughness " << roughness << ", cosine " << cosine) {
                const bsdf::Bsdf lobe = translucent(roughness);
                const glm::vec3 outgoing = towards(cosine, 0.5F);
                const Fit fit = densityFit(lobe, outgoing, 400000, seed++);
                BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
                BOOST_TEST(furnace(lobe, outgoing, 200000, seed++) == 1.0F, boost::test_tools::tolerance(0.01F));
                const bsdf::Sample drawn = lobe.sample(outgoing, glm::vec2 { 0.3F, 0.6F }, 0.5F);
                BOOST_TEST(drawn.found());
                BOOST_TEST((drawn.incoming.z * outgoing.z < 0.0F));
                BOOST_TEST((drawn.lobe == bsdf::Lobe::Transmission));
            }
        }
    }
}

/**
 * The mean colour of a white dome seen through a sphere of an OpenPBR surface of some inputs, over
 * a count of samples. Its depth limits are raised so nothing ends a path but the walk's limit.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the walk's limit, then the samples
glm::vec3 furnaceOf(const std::vector<std::pair<std::string, scene::Value>> & inputs, std::int32_t walk = kEndless, std::int32_t samples = 256) {
    return meanColour(pictured(samples, [&](scene::Scene & scene, const scene::Handle & options) {
        for (const char * limit : { "maxDepth", "diffuseDepth", "glossyDepth", "transmissionDepth" }) {
            scene.set(options, limit, scene::Value::integer(1024));
        }
        scene.set(options, "subsurfaceDepth", scene::Value::integer(walk));
        if (!inputs.empty()) {
            ball(scene, "ball", 1.0F, openPbr(scene, "white", inputs));
        }
    }));
}

// a white subsurface sphere in a white dome returns the dome's light, rough or smooth, and so does
// a white thin wall that scatters forwards
BOOST_AUTO_TEST_CASE(moya_subsurface_a_white_subsurface_returns_a_furnaces_light_test) {
    const glm::vec3 bare = furnaceOf({});
    for (const float roughness : { 0.0F, 0.3F }) {
        BOOST_TEST_CONTEXT("roughness " << roughness) {
            const glm::vec3 white = furnaceOf({ { "subsurface_weight", scene::Value::number(1.0F) },
                { "subsurface_color", scene::Value::colour(glm::vec3 { 1.0F }) }, { "subsurface_radius", scene::Value::number(0.5F) },
                { "specular_roughness", scene::Value::number(roughness) } });
            BOOST_TEST(near(white, bare, 0.02F), white.x << " " << white.y << " " << white.z << " against " << bare.x << " " << bare.y << " " << bare.z);
        }
    }
    const glm::vec3 sheet = furnaceOf({ { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(glm::vec3 { 1.0F }) },
        { "subsurface_scatter_anisotropy", scene::Value::number(0.5F) }, { "geometry_thin_walled", scene::Value::integer(1) } });
    BOOST_TEST(near(sheet, bare, 0.02F), sheet.x << " " << sheet.y << " " << sheet.z << " against " << bare.x << " " << bare.y << " " << bare.z);
}

// a walk leaves its surface by a Lambertian lobe, so light samples alone find the light it carries
// out. A white subsurface under light samples alone returns a white dome's light, smooth or rough
BOOST_AUTO_TEST_CASE(moya_subsurface_light_samples_alone_find_a_walks_light_test) {
    const glm::vec3 bare = furnaceOf({});
    for (const float roughness : { 0.0F, 0.3F }) {
        BOOST_TEST_CONTEXT("roughness " << roughness) {
            const glm::vec3 white = meanColour(pictured(256, [&](scene::Scene & scene, const scene::Handle & options) {
                for (const char * limit : { "maxDepth", "diffuseDepth", "glossyDepth", "transmissionDepth" }) {
                    scene.set(options, limit, scene::Value::integer(1024));
                }
                scene.set(options, "lightSampling", scene::Value::string("light"));
                ball(scene, "ball", 1.0F,
                    openPbr(scene, "white", { { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(glm::vec3 { 1.0F }) },
                                                { "subsurface_radius", scene::Value::number(0.5F) }, { "specular_roughness", scene::Value::number(roughness) } }));
            }));
            BOOST_TEST(near(white, bare, 0.03F), white.x << " " << white.y << " " << white.z << " against " << bare.x << " " << bare.y << " " << bare.z);
        }
    }
}

/**
 * A white subsurface ball of a roughness, lit by a distant light behind the camera alone, as the
 * mean of its picture.
 **/
glm::vec3 underSunOf(float roughness) {
    return meanColour(pictured(256, [&](scene::Scene & scene, const scene::Handle &) {
        // the dome is made before build() runs, and found once applied
        scene.commit();
        scene.apply();
        scene.set(scene.objects("domeLight").front(), "intensity", scene::Value::number(0.0F));
        // a distant light of no angle gives an irradiance of its intensity. It shines down its own
        // -z, and a half turn about x turns it to shine along +z, as the camera looks
        const scene::Handle sun = scene.create("distantLight", "sun");
        scene.set(sun, "angle", scene::Value::number(0.0F));
        scene.set(sun, "transform", scene::Value::matrix(glm::rotate(glm::mat4(1.0F), glm::radians(180.0F), glm::vec3 { 1.0F, 0.0F, 0.0F })));
        ball(scene, "ball", 1.0F,
            openPbr(scene, "white", { { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(glm::vec3 { 1.0F }) },
                                        { "subsurface_radius", scene::Value::number(0.5F) }, { "specular_roughness", scene::Value::number(roughness) } }));
    }));
}

// a smooth subsurface is lit by a light of no size, through the light sample where its walk leaves,
// as a rough one is. The two differ only where light enters, so they lie near each other
BOOST_AUTO_TEST_CASE(moya_subsurface_a_smooth_walk_is_lit_by_a_distant_light_test) {
    const glm::vec3 rough = underSunOf(0.8F);
    const glm::vec3 smooth = underSunOf(0.0F);
    BOOST_TEST(rough.y > 0.05F, rough.y);
    BOOST_TEST(near(smooth, rough, 0.1F), smooth.x << " " << smooth.y << " " << smooth.z << " against " << rough.x << " " << rough.y << " " << rough.z);
}

// a walk is absorbed at the options' limit, so a white subsurface that may not walk at all is dark
// but for its reflection
BOOST_AUTO_TEST_CASE(moya_subsurface_a_walk_is_absorbed_at_its_limit_test) {
    const glm::vec3 bare = furnaceOf({});
    const glm::vec3 held = furnaceOf({ { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(glm::vec3 { 1.0F }) },
                                         { "subsurface_radius", scene::Value::number(0.5F) } },
        0);
    BOOST_TEST(held.y < 0.2F * bare.y, held.y << " against " << bare.y);
}

/**
 * Chandrasekhar's H-function of a single-scattering albedo for isotropic scattering, at a cosine,
 * by iterating its equation over a midpoint rule until it settles.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the albedo, then the cosine
double chandrasekhar(double albedo, double cosine) {
    constexpr int kSteps = 2000;
    std::vector<double> estimate(kSteps + 1, 1.0);
    const auto at = [&](double mu) {
        double sum = 0.0;
        for (int i = 0; i < kSteps; ++i) {
            const double other = (i + 0.5) / kSteps;
            sum += estimate[static_cast<std::size_t>(i)] / (mu + other);
        }
        return 1.0 / (1.0 - (0.5 * albedo * mu * sum / kSteps));
    };
    for (int round = 0; round < 200; ++round) {
        std::vector<double> next(kSteps + 1);
        for (int i = 0; i < kSteps; ++i) {
            next[static_cast<std::size_t>(i)] = at((i + 0.5) / kSteps);
        }
        estimate.swap(next);
    }
    return at(cosine);
}

// a grey subsurface whose mean free path is far shorter than the sphere, behind a boundary of no
// bend, is a half-space that scatters evenly. Seen along its normal under a white dome, it
// reflects one less the root of one less its albedo times Chandrasekhar's H at a cosine of one,
// for the albedo the mapping gives its colour
BOOST_AUTO_TEST_CASE(moya_subsurface_a_dense_subsurface_reflects_as_a_half_space_test) {
    const glm::vec3 bare = furnaceOf({});
    for (const float grey : { 0.2F, 0.5F, 0.8F }) {
        BOOST_TEST_CONTEXT("grey " << grey) {
            const glm::vec3 seen = furnaceOf({ { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(glm::vec3 { grey }) },
                { "subsurface_radius", scene::Value::number(0.01F) }, { "subsurface_radius_scale", scene::Value::colour(glm::vec3 { 1.0F }) },
                { "specular_ior", scene::Value::number(1.0F) } },
                kEndless, 1024);
            const double albedo = volume::openpbr::singleAlbedo(grey, 0.0F);
            const auto reflected = static_cast<float>(1.0 - (std::sqrt(1.0 - albedo) * chandrasekhar(albedo, 1.0)));
            BOOST_TEST(near(seen, bare * reflected, 0.02F), seen.x << " " << seen.y << " " << seen.z << " against " << reflected * bare.x);
        }
    }
}

// the media a surface encloses are made from its colours once they are converted into ACEScg, so
// a subsurface colour and a transmission colour given in lin_rec709 make the media of their
// ACEScg colours
BOOST_AUTO_TEST_CASE(moya_subsurface_the_media_take_their_colours_in_acescg_test) {
    const glm::vec3 red { 0.9F, 0.3F, 0.1F };
    const glm::vec3 blue { 0.2F, 0.4F, 0.8F };
    scene::Scene scene(scene::kinds());
    scene.create("options", "options");
    ball(scene, "ball", 1.0F,
        openPbr(scene, "skin",
            { { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(red, "lin_rec709") },
                { "transmission_weight", scene::Value::number(0.5F) }, { "transmission_color", scene::Value::colour(blue, "lin_rec709") },
                { "transmission_depth", scene::Value::number(2.0F) } }));
    scene.commit();
    scene.apply();
    const v3d::moya::colour::Config config;
    const geometry::ConvertedMaterials converted = geometry::ConvertedMaterials::of(geometry::gather(scene, {}), config);
    const auto skin = std::ranges::find_if(converted.materials, [](const kernel::surface::Material & material) { return material.subsurfaces != 0U; });
    BOOST_TEST_REQUIRE((skin != converted.materials.end()));
    kernel::surface::OpenPbr expected;
    expected.subsurfaceWeight = 1.0F;
    expected.subsurfaceColour = config.input("lin_rec709").apply(red);
    expected.transmissionColour = config.input("lin_rec709").apply(blue);
    expected.transmissionDepth = 2.0F;
    const volume::Medium walked = volume::openpbr::walked(expected);
    const volume::Medium inside = volume::openpbr::interior(expected);
    for (glm::length_t i = 0; i < 3; ++i) {
        BOOST_TEST(skin->walked.lobes[0].scattering[i] == walked.lobes[0].scattering[i], boost::test_tools::tolerance(1.0e-4F));
        BOOST_TEST(skin->medium.absorption[i] == inside.absorption[i], boost::test_tools::tolerance(1.0e-4F));
    }
}

// the options' subsurfaceDepth limits a walk's events, 256 unless it is set
BOOST_AUTO_TEST_CASE(moya_subsurface_the_options_limit_a_walk_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "options");
    scene.commit();
    scene.apply();
    BOOST_TEST(integrate::settingsOf(scene).subsurfaceDepth == 256U);
    scene.set(options, "subsurfaceDepth", scene::Value::integer(9));
    scene.commit();
    scene.apply();
    BOOST_TEST(integrate::settingsOf(scene).subsurfaceDepth == 9U);
}

};  // namespace

};  // namespace v3d::moya::tracing

