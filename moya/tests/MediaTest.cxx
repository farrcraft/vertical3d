/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/integrate/media/Enclosure.h>
#include <moya/libmoya/kernel/integrate/media/Media.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Homogeneous.h>
#include <moya/libmoya/kernel/volume/openpbr/Interior.h>
#include <moya/libmoya/kernel/volume/Lobe.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/Phase.h>
#include <moya/libmoya/kernel/volume/PhaseSample.h>
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
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
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
namespace media = v3d::moya::kernel::integrate::media;
namespace scene = v3d::moya::scene;
namespace spectrum = v3d::moya::kernel::spectrum;
namespace volume = v3d::moya::kernel::volume;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 24;

/** The side of each picture of a medium, in pixels. **/
constexpr std::uint32_t kSmall = 8;

/** The wavelengths drawn to give the colour a light at each wavelength makes on average. **/
constexpr int kDraws = 4096;

/** A closed surface of a key, a priority and an index. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the key, the priority, then the index
media::Enclosure enclosure(std::uint32_t key, std::int32_t priority, float index) {
    return media::Enclosure { .key = key, .priority = priority, .index = index };
}

/** A direction of a polar cosine about z, turned about it. A negative cosine lies below. **/
glm::vec3 towards(float cosine, float turn) {
    const float across = std::sqrt(1.0F - (cosine * cosine));
    return glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine };
}

/**
 * The BSDF of a smooth OpenPBR surface that transmits everything it does not reflect, of an index,
 * shaded about +z, with a medium of an index on the side +z faces.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the surface's index, then the outside's
bsdf::Bsdf clear(float ior, float outside, const glm::vec3 & outgoing) {
    bsdf::Bsdf out;
    out.frame = v3d::moya::kernel::surface::Frame::around(glm::vec3 { 0.0F, 0.0F, 1.0F });
    out.geometric = glm::vec3 { 0.0F, 0.0F, 1.0F };
    out.albedos = v3d::moya::material::ggxAlbedos();
    out.outside = outside;
    v3d::moya::kernel::surface::OpenPbr surface;
    surface.specularIor = ior;
    surface.specularRoughness = 0.0F;
    surface.transmissionWeight = 1.0F;
    bsdf::Layering::add(out, surface, colour::acescgTable(), spectrum::SampledWavelengths::visible(0.5F), std::fabs(outgoing.z));
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

/** A material of a terminal and the node that gives it. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the terminal
scene::Handle materialOf(scene::Scene & scene, const std::string & name, const std::string & terminal, const scene::Handle & shader) {
    const scene::Handle out = scene.create("material", name);
    scene.set(out, terminal, scene::Value::handle(shader));
    return out;
}

/** A material of an OpenPBR glass of an index, which transmits, thin-walled or not. **/
scene::Handle glass(scene::Scene & scene, const std::string & name, float ior, bool thin = false) {
    return materialOf(scene, name, "surface",
        node(scene, name, kOpenPbr,
            { { "transmission_weight", scene::Value::number(1.0F) }, { "specular_ior", scene::Value::number(ior) },
                { "geometry_thin_walled", scene::Value::integer(thin ? 1 : 0) } }));
}

/**
 * A closed sphere of a radius at the origin, placed with a material and a priority, and turned
 * as given. Its cuts are set to its poles, since RI's cut a sphere at one unit from its centre.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the radius
scene::Handle ball(scene::Scene & scene, const std::string & name, float radius, const scene::Handle & material, std::int32_t priority,
    const glm::mat4 & placed = glm::mat4(1.0F)) {
    const scene::Handle sphere = scene.create("sphere", name);
    scene.set(sphere, "radius", scene::Value::number(radius));
    scene.set(sphere, "zmin", scene::Value::number(-radius));
    scene.set(sphere, "zmax", scene::Value::number(radius));
    const scene::Handle out = instanced(scene, name, sphere, {}, placed);
    scene.set(out, "material", scene::Value::handle(material));
    scene.set(out, "priority", scene::Value::integer(priority));
    return out;
}

/**
 * A camera five units back along -z looking at the origin, under a white dome light of no image,
 * with a grey square two units behind the origin to see through what lies between.
 **/
void framed(scene::Scene & scene) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(8));
    scene.create("domeLight", "sky");
    scene.set(camera, "fov", scene::Value::number(30.0F));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    const scene::Handle floor = scene.create("mesh", "floor");
    scene.set(floor, "P", scene::Value::points({ { -1.0F, -3.0F, 2.0F }, { 3.0F, -3.0F, 2.0F }, { 3.0F, 3.0F, 2.0F }, { -1.0F, 3.0F, 2.0F } }));
    scene.set(floor, "vertices", scene::Value::integers({ 3, 3 }));
    scene.set(floor, "indices", scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
    instanced(scene, "floor", floor, {}, glm::mat4(1.0F));
}

/** The picture of a scene that build() fills after framed(). **/
template <typename F>
film::Snapshot rendered(F && build) {
    scene::Scene scene(scene::kinds());
    framed(scene);
    build(scene);
    scene.commit();
    scene.apply();
    CpuDevice device;
    return integrate::render(device, scene, {}, v3d::moya::colour::Config());
}

/** Whether two snapshots hold the same floats, bit for bit, and the same counts of samples. **/
bool sameBytes(const film::Snapshot & one, const film::Snapshot & other) {
    return one.values.size() == other.values.size() && one.samples == other.samples &&
        std::memcmp(one.values.data(), other.values.data(), one.values.size() * sizeof(float)) == 0;
}

// the medium a path is in is that of the surface of highest priority it is inside, and of equal
// priorities the one it entered last. Beyond a surface lies that medium, leaving the surface out
BOOST_AUTO_TEST_CASE(moya_media_the_medium_is_the_highest_priority_entered_last_test) {
    media::Media inside;
    BOOST_TEST(inside.outside(1U) == 1.0F);
    BOOST_TEST(!inside.outranked(0));
    inside.enter(enclosure(1U, 0, 1.33F));
    inside.enter(enclosure(2U, 0, 1.5F));
    BOOST_TEST(inside.outside(9U) == 1.5F);
    BOOST_TEST(inside.outside(2U) == 1.33F);
    BOOST_TEST(!inside.outranked(0));
    inside.enter(enclosure(3U, 2, 1.2F));
    inside.enter(enclosure(4U, 1, 1.7F));
    BOOST_TEST(inside.outside(9U) == 1.2F);
    BOOST_TEST(inside.outside(3U) == 1.7F);
    BOOST_TEST(inside.outranked(1));
    BOOST_TEST(!inside.outranked(2));
    inside.enter(enclosure(3U, 2, 1.2F));
    BOOST_TEST(inside.count == 4U);
    inside.leave(3U);
    inside.leave(8U);
    BOOST_TEST(inside.count == 3U);
    BOOST_TEST(inside.outside(9U) == 1.7F);
    inside.cross(enclosure(4U, 1, 1.7F), false);
    inside.cross(enclosure(5U, 0, 1.1F), true);
    BOOST_TEST(inside.outside(9U) == 1.1F);
}

// a path inside as many surfaces as it holds leaves the one of lowest priority, of those the one
// it entered first, as it enters another
BOOST_AUTO_TEST_CASE(moya_media_a_path_too_deep_leaves_its_lowest_priority_test) {
    media::Media inside;
    for (std::uint32_t key = 0; key < media::kMediaDepth; ++key) {
        inside.enter(enclosure(key, key == 3U || key == 5U ? -1 : 1, 1.0F + (0.1F * static_cast<float>(key))));
    }
    BOOST_TEST(inside.count == media::kMediaDepth);
    inside.enter(enclosure(100U, 0, 1.25F));
    BOOST_TEST(inside.count == media::kMediaDepth);
    BOOST_TEST(inside.find(3U) == media::kNotInside);
    BOOST_TEST(inside.find(5U) != media::kNotInside);
    BOOST_TEST(inside.find(100U) == media::kMediaDepth - 1U);
}

// a material encloses a medium when a path may cross into it: a glass that is not thin-walled,
// or whose transmission a node gives, and not an opaque one or a thin wall. A material of a
// volume and no surface is a boundary. An instance's priority reaches the instances shading reads
BOOST_AUTO_TEST_CASE(moya_media_a_material_encloses_what_a_path_may_cross_into_test) {
    scene::Scene scene(scene::kinds());
    framed(scene);
    ball(scene, "glass", 1.0F, glass(scene, "glass", 1.33F), 3);
    ball(scene, "thin", 1.0F, glass(scene, "thin", 1.5F, true), 0);
    ball(scene, "opaque", 1.0F, materialOf(scene, "opaque", "surface", node(scene, "opaque", kOpenPbr, {})), 0);
    const scene::Handle fed = node(scene, "fed", kOpenPbr, {});
    const scene::Handle across = node(scene, "across", "ND_extract_vector2", { { "index", scene::Value::integer(0) } });
    const scene::Handle first = scene.create("connection", "st into across");
    scene.set(first, "from", scene::Value::handle(node(scene, "uv", "ND_texcoord_vector2", {})));
    scene.set(first, "to", scene::Value::handle(across));
    scene.set(first, "input", scene::Value::string("in"));
    const scene::Handle second = scene.create("connection", "across into transmission");
    scene.set(second, "from", scene::Value::handle(across));
    scene.set(second, "to", scene::Value::handle(fed));
    scene.set(second, "input", scene::Value::string("transmission_weight"));
    ball(scene, "fed", 1.0F, materialOf(scene, "fed", "surface", fed), 0);
    ball(scene, "fog", 1.0F, materialOf(scene, "fog", "volume", node(scene, "fog", "ND_volume", {})), 0);
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 5U);
    const auto & materials = prototypes.materials;
    BOOST_TEST(materials[0].encloses());
    BOOST_TEST(media::indexOf(materials[0]) == 1.33F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(!materials[1].encloses());
    BOOST_TEST(!materials[2].encloses());
    BOOST_TEST(materials[3].transmits == 1U);
    BOOST_TEST(materials[3].encloses());
    BOOST_TEST(materials[4].boundary == 1U);
    BOOST_TEST(materials[4].encloses());
    BOOST_TEST(materials[4].cutOut());
    BOOST_TEST(media::indexOf(materials[4]) == 1.0F);
    std::vector<std::int32_t> priorities;
    priorities.reserve(prototypes.instances.size());
    for (const geometry::Instance & instance : prototypes.instances) {
        priorities.push_back(instance.priority);
    }
    BOOST_TEST((std::count(priorities.begin(), priorities.end(), 3) == 1));
}

// a smooth dielectric bends light by its own index over that of the medium beyond it, by Snell's
// law, from either side and whichever index is the greater. Water of 1.33 against glass of 1.5
// reflects everything beyond its critical angle
BOOST_AUTO_TEST_CASE(moya_media_a_dielectric_bends_by_the_ratio_of_the_media_on_its_sides_test) {
    const float sine = std::sqrt(1.0F - (0.766F * 0.766F));
    const auto bent = [](float ratio, float sine) {
        const float across = sine / ratio;
        return std::sqrt(1.0F - (across * across));
    };
    for (const auto & [ior, outside] : { std::pair { 1.33F, 1.5F }, std::pair { 1.5F, 1.33F }, std::pair { 1.33F, 1.0F } }) {
        BOOST_TEST_CONTEXT(ior << " in " << outside) {
            const glm::vec3 above = towards(0.766F, 0.3F);
            const bsdf::Sample entering = clear(ior, outside, above).sample(above, { 0.5F, 0.5F }, 0.999F);
            BOOST_TEST_REQUIRE(entering.found());
            BOOST_TEST((entering.lobe == bsdf::Lobe::Transmission));
            BOOST_TEST(entering.incoming.z == -bent(ior / outside, sine), boost::test_tools::tolerance(1.0e-4F));
            const glm::vec3 below = towards(-0.766F, 0.3F);
            const bsdf::Sample leaving = clear(ior, outside, below).sample(below, { 0.5F, 0.5F }, 0.999F);
            BOOST_TEST_REQUIRE(leaving.found());
            BOOST_TEST((leaving.lobe == bsdf::Lobe::Transmission));
            BOOST_TEST(leaving.incoming.z == bent(outside / ior, sine), boost::test_tools::tolerance(1.0e-4F));
        }
    }
    const glm::vec3 grazing = towards(0.34F, 0.3F);
    const bsdf::Sample trapped = clear(1.33F, 1.5F, grazing).sample(grazing, { 0.5F, 0.5F }, 0.999F);
    BOOST_TEST_REQUIRE(trapped.found());
    BOOST_TEST(trapped.incoming.z > 0.0F);
    BOOST_TEST(trapped.value[0] == 1.0F, boost::test_tools::tolerance(1.0e-4F));
}

// a liquid of lower priority inside a glass is passed by, so the picture is the glass's alone to
// the byte. Of equal priorities the liquid shows
BOOST_AUTO_TEST_CASE(moya_media_a_surface_inside_a_medium_of_higher_priority_is_passed_test) {
    const auto picture = [](bool liquid, std::int32_t priority) {
        return rendered([&](scene::Scene & scene) {
            ball(scene, "glass", 1.5F, glass(scene, "glass", 1.5F), priority);
            if (liquid) {
                ball(scene, "liquid", 0.5F, glass(scene, "liquid", 1.33F), 0);
            }
        });
    };
    const film::Snapshot alone = picture(false, 1);
    BOOST_TEST(sameBytes(alone, picture(true, 1)));
    BOOST_TEST(!sameBytes(picture(false, 0), picture(true, 0)));
}

// a camera inside a glass of higher priority starts in its medium, so a liquid of lower priority
// in front of it is passed by. A boundary, a volume's surface of no BSDF, is passed by too
BOOST_AUTO_TEST_CASE(moya_media_a_camera_inside_a_medium_starts_in_it_test) {
    const auto picture = [](bool liquid, std::int32_t priority) {
        return rendered([&](scene::Scene & scene) {
            ball(scene, "room", 20.0F, glass(scene, "room", 1.5F), priority);
            if (liquid) {
                ball(scene, "liquid", 0.5F, glass(scene, "liquid", 1.33F), 0);
            }
        });
    };
    BOOST_TEST(sameBytes(picture(false, 1), picture(true, 1)));
    BOOST_TEST(!sameBytes(picture(false, 0), picture(true, 0)));
    const film::Snapshot bare = rendered([](scene::Scene &) {});
    const film::Snapshot fogged = rendered([](scene::Scene & scene) {
        ball(scene, "fog", 1.0F, materialOf(scene, "fog", "volume", node(scene, "fog", "ND_volume", {})), 0);
    });
    BOOST_TEST(sameBytes(bare, fogged));
}

/** A sphere turned so a camera on the z axis looks at its equator, away from its poles. **/
glm::mat4 turned() {
    return glm::rotate(glm::mat4(1.0F), glm::radians(90.0F), glm::vec3 { 1.0F, 0.0F, 0.0F });
}

/** Connects an output to an input, by a connection of a name. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the input
void connect(scene::Scene & scene, const std::string & name, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", name);
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** A material of a volume of one anisotropic VDF, of an absorption, scattering and anisotropy. **/
scene::Handle fogOf(scene::Scene & scene, const std::string & name, const glm::vec3 & absorption, const glm::vec3 & scattering, float anisotropy) {
    const scene::Handle vdf = node(scene, name + " vdf", "ND_anisotropic_vdf",
        { { "absorption", scene::Value::vector(absorption) }, { "scattering", scene::Value::vector(scattering) }, { "anisotropy", scene::Value::number(anisotropy) } });
    const scene::Handle shader = node(scene, name, "ND_volume", {});
    connect(scene, name + " into volume", vdf, shader, "vdf");
    return materialOf(scene, name, "volume", shader);
}

/**
 * An orthographic camera five units back along -z looking at the origin, of a count of samples.
 * Its screen is a tenth of a unit across, so every ray runs along z within a twentieth of the
 * axis.
 **/
scene::Handle aimed(scene::Scene & scene, std::int32_t samples) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSmall, kSmall }));
    scene.set(options, "samples", scene::Value::integer(samples));
    scene.set(camera, "screen", scene::Value::floats({ -0.05F, 0.05F, -0.05F, 0.05F }));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    return options;
}

/** The picture of a scene that build() fills, given the options aimed() makes. **/
template <typename F>
film::Snapshot pictured(std::int32_t samples, F && build) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = aimed(scene, samples);
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

/**
 * The ACEScg colour a light makes on average, given its radiance at the wavelengths a path
 * carries, as the film adds it, over evenly spread draws of the wavelengths.
 **/
template <typename F>
glm::vec3 colourOf(F && radiance) {
    glm::dvec3 sum { 0.0 };
    for (int i = 0; i < kDraws; ++i) {
        const auto wavelengths = spectrum::SampledWavelengths::visible((static_cast<float>(i) + 0.5F) / static_cast<float>(kDraws));
        sum += glm::dvec3(spectrum::acescgFromXyz(spectrum::toXyz(radiance(wavelengths), wavelengths)));
    }
    return glm::vec3(sum / static_cast<double>(kDraws));
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

/** A medium's coefficients at a set of wavelengths. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the absorption, then the scattering
volume::Coefficients coefficientsAt(const glm::vec3 & absorption, const glm::vec3 & scattering, float anisotropy, const spectrum::SampledWavelengths & wavelengths) {
    volume::Medium medium;
    medium.absorption = absorption;
    medium.lobes[0] = volume::Lobe { .scattering = scattering, .anisotropy = anisotropy };
    return volume::coefficientsOf(colour::acescgTable(), medium, wavelengths);
}

/**
 * How well directions drawn by a draw function follow a density about the direction a path arrives
 * along, by chi-squared over bins even in the cosine to it and the turn about it. Each bin expects
 * the density integrated over it on a finer grid, and bins expected to hold fewer than five are
 * pooled.
 **/
template <typename D, typename S>
Fit phaseFit(const glm::vec3 & forward, D && density, S && draw, std::uint32_t seed) {
    constexpr std::size_t kCosines = 40;
    constexpr std::size_t kTurns = 20;
    constexpr std::size_t kFiner = 64;
    constexpr std::size_t kDrawn = 400000;
    constexpr double kTau = 6.283185307179586;
    const v3d::moya::kernel::surface::Frame frame = v3d::moya::kernel::surface::Frame::around(forward);
    std::vector<double> counts(kCosines * kTurns, 0.0);
    for (std::size_t i = 0; i < kDrawn; ++i) {
        const glm::vec2 u { next(seed), next(seed) };
        const glm::vec3 local = frame.toLocal(draw(u, next(seed)));
        const auto row = std::min(kCosines - 1U, static_cast<std::size_t>((local.z + 1.0F) * 0.5F * static_cast<float>(kCosines)));
        const double turn = (std::atan2(local.y, local.x) + (kTau / 2.0)) / kTau;
        const auto column = std::min(kTurns - 1U, static_cast<std::size_t>(turn * static_cast<double>(kTurns)));
        counts[(row * kTurns) + column] += 1.0;
    }
    const double cosineStep = 2.0 / static_cast<double>(kCosines * kFiner);
    double pooledCount = 0.0;
    double pooledExpected = 0.0;
    Fit out;
    for (std::size_t row = 0; row < kCosines; ++row) {
        double integral = 0.0;
        for (std::size_t finer = 0; finer < kFiner; ++finer) {
            const double cosine = -1.0 + (cosineStep * (static_cast<double>((row * kFiner) + finer) + 0.5));
            integral += static_cast<double>(density(static_cast<float>(cosine))) * cosineStep * (kTau / static_cast<double>(kTurns));
        }
        for (std::size_t column = 0; column < kTurns; ++column) {
            const double expected = integral * static_cast<double>(kDrawn);
            const double counted = counts[(row * kTurns) + column];
            if (expected < 5.0) {
                pooledExpected += expected;
                pooledCount += counted;
                continue;
            }
            out.statistic += static_cast<float>((counted - expected) * (counted - expected) / expected);
            ++out.freedom;
        }
    }
    if (pooledExpected > 0.0) {
        out.statistic += static_cast<float>((pooledCount - pooledExpected) * (pooledCount - pooledExpected) / pooledExpected);
        ++out.freedom;
    }
    if (out.freedom > 0) {
        --out.freedom;
    }
    return out;
}

// Henyey and Greenstein's phase function integrates to one over the sphere, and the directions it
// draws follow it, back, even and onwards
BOOST_AUTO_TEST_CASE(moya_media_henyey_greensteins_samples_follow_its_density_test) {
    const glm::vec3 forward = glm::normalize(glm::vec3 { 0.3F, -0.5F, 0.8F });
    for (const float anisotropy : { -0.6F, 0.0F, 0.4F, 0.9F }) {
        BOOST_TEST_CONTEXT("anisotropy " << anisotropy) {
            constexpr int kSteps = 20000;
            double integral = 0.0;
            for (int i = 0; i < kSteps; ++i) {
                const float cosine = -1.0F + ((2.0F * (static_cast<float>(i) + 0.5F)) / static_cast<float>(kSteps));
                integral += static_cast<double>(volume::henyeyGreenstein(cosine, anisotropy)) * (2.0 / kSteps) * 6.283185307179586;
            }
            BOOST_TEST(integral == 1.0, boost::test_tools::tolerance(2.0e-3));
            const Fit fit = phaseFit(
                forward, [&](float cosine) { return volume::henyeyGreenstein(cosine, anisotropy); },
                [&](const glm::vec2 & u, float) { return volume::henyeyGreensteinSample(forward, anisotropy, u); }, 7U);
            BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
        }
    }
}

// a medium of two lobes draws a direction by each lobe's share of what it scatters over every
// wavelength, so the density is the same whichever wavelength is tracked. Each wavelength's phase
// function weighs the lobes by its own scattering and integrates to one
BOOST_AUTO_TEST_CASE(moya_media_a_phase_function_of_two_lobes_follows_its_density_test) {
    const auto wavelengths = spectrum::SampledWavelengths::visible(0.37F);
    volume::Medium medium;
    medium.lobes[0] = volume::Lobe { .scattering = glm::vec3 { 0.9F, 0.2F, 0.1F }, .anisotropy = 0.7F };
    medium.lobes[1] = volume::Lobe { .scattering = glm::vec3 { 0.1F, 0.4F, 0.8F }, .anisotropy = -0.4F };
    const volume::Coefficients coefficients = volume::coefficientsOf(colour::acescgTable(), medium, wavelengths);
    const glm::vec3 forward = glm::normalize(glm::vec3 { -0.2F, 0.6F, 0.4F });
    const Fit fit = phaseFit(
        forward, [&](float cosine) { return volume::phaseDensity(coefficients, cosine); },
        [&](const glm::vec2 & u, float choice) {
            const volume::PhaseSample drawn = volume::phaseSample(coefficients, forward, u, choice);
            BOOST_TEST(drawn.density == volume::phaseDensity(coefficients, glm::dot(forward, drawn.incoming)), boost::test_tools::tolerance(1.0e-4F));
            return drawn.incoming;
        },
        11U);
    BOOST_TEST(fit.passes(), "chi-squared " << fit.statistic << " over " << fit.freedom);
    constexpr int kSteps = 20000;
    spectrum::SampledSpectrum integral(0.0F);
    for (int i = 0; i < kSteps; ++i) {
        const float cosine = -1.0F + ((2.0F * (static_cast<float>(i) + 0.5F)) / static_cast<float>(kSteps));
        integral += volume::phaseValue(coefficients, cosine) * (2.0F * 6.2831853F / static_cast<float>(kSteps));
    }
    for (int i = 0; i < spectrum::kSamples; ++i) {
        BOOST_TEST(integral[i] == 1.0F, boost::test_tools::tolerance(2.0e-3F));
    }
}

// Beer and Lambert's law: a homogeneous medium lets through e to the minus its extinction times
// the distance, at each wavelength, and over an endless distance only where it is clear
BOOST_AUTO_TEST_CASE(moya_media_transmittance_is_beer_and_lamberts_test) {
    spectrum::SampledSpectrum extinction(0.0F);
    extinction[0] = 0.5F;
    extinction[1] = 2.0F;
    const spectrum::SampledSpectrum passed = volume::transmittance(extinction, 1.5F);
    BOOST_TEST(passed[0] == std::exp(-0.75F), boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(passed[1] == std::exp(-3.0F), boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(passed[2] == 1.0F);
    const spectrum::SampledSpectrum endless = volume::transmittance(extinction, std::numeric_limits<float>::infinity());
    BOOST_TEST(endless[0] == 0.0F);
    BOOST_TEST(endless[2] == 1.0F);
    BOOST_TEST(std::isinf(volume::freeFlight(0.0F, 0.5F)));
    BOOST_TEST(volume::freeFlight(2.0F, 0.5F) == std::log(2.0F) / 2.0F, boost::test_tools::tolerance(1.0e-6F));
}

// a sphere of a medium that absorbs by colour, in front of a white dome, lets through Beer and
// Lambert's share along its diameter at each wavelength
BOOST_AUTO_TEST_CASE(moya_media_an_absorbing_sphere_lets_through_beer_and_lamberts_share_test) {
    const glm::vec3 absorption { 0.2F, 0.6F, 1.2F };
    const film::Snapshot snapshot = pictured(256, [&](scene::Scene & scene, const scene::Handle &) {
        scene.create("domeLight", "sky");
        ball(scene, "fog", 1.0F, fogOf(scene, "fog", absorption, glm::vec3 { 0.0F }, 0.0F), 0, turned());
    });
    const glm::vec3 expected = colourOf([&](const spectrum::SampledWavelengths & wavelengths) {
        const volume::Coefficients coefficients = coefficientsAt(absorption, glm::vec3 { 0.0F }, 0.0F, wavelengths);
        return volume::transmittance(coefficients.extinction(), 2.0F) * spectrum::illuminant(colour::acescgTable(), glm::vec3 { 1.0F }, wavelengths);
    });
    BOOST_TEST(near(meanColour(snapshot), expected, 0.02F), meanColour(snapshot).x << " " << meanColour(snapshot).y << " " << meanColour(snapshot).z << " against "
        << expected.x << " " << expected.y << " " << expected.z);
}

// a sphere of a medium, lit from behind the camera by a distant light and scattering once at most,
// sends back what single scattering's closed form gives along its diameter, at each wavelength.
// That is the scattering times the phase function back towards the light, times the light, times
// one less e to the minus twice the extinction over the diameter, over twice the extinction
BOOST_AUTO_TEST_CASE(moya_media_single_scattering_under_a_distant_light_has_its_closed_form_test) {
    const glm::vec3 absorption { 0.1F, 0.3F, 0.2F };
    const glm::vec3 scattering { 0.4F, 0.9F, 1.6F };
    constexpr float kAnisotropy = 0.4F;
    const film::Snapshot snapshot = pictured(512, [&](scene::Scene & scene, const scene::Handle & options) {
        scene.set(options, "volumeDepth", scene::Value::integer(1));
        const scene::Handle sun = scene.create("distantLight", "sun");
        scene.set(sun, "angle", scene::Value::number(0.0F));
        scene.set(sun, "transform", scene::Value::matrix(glm::rotate(glm::mat4(1.0F), glm::radians(180.0F), glm::vec3 { 1.0F, 0.0F, 0.0F })));
        ball(scene, "fog", 1.0F, fogOf(scene, "fog", absorption, scattering, kAnisotropy), 0, turned());
    });
    const glm::vec3 expected = colourOf([&](const spectrum::SampledWavelengths & wavelengths) {
        const volume::Coefficients coefficients = coefficientsAt(absorption, scattering, kAnisotropy, wavelengths);
        const spectrum::SampledSpectrum extinction = coefficients.extinction();
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            out[i] = coefficients.scattering[0][i] * volume::henyeyGreenstein(-1.0F, kAnisotropy) * (1.0F - std::exp(-4.0F * extinction[i])) / (2.0F * extinction[i]);
        }
        return out * spectrum::illuminant(colour::acescgTable(), glm::vec3 { 1.0F }, wavelengths);
    });
    BOOST_TEST(near(meanColour(snapshot), expected, 0.03F), meanColour(snapshot).x << " " << meanColour(snapshot).y << " " << meanColour(snapshot).z << " against "
        << expected.x << " " << expected.y << " " << expected.z);
}

// a sphere of a medium that scatters by colour and absorbs nothing, inside a white dome, returns
// the dome's light, however many times a path scatters in it
BOOST_AUTO_TEST_CASE(moya_media_a_medium_that_only_scatters_returns_a_furnaces_light_test) {
    const auto furnace = [](bool fogged) {
        return pictured(256, [&](scene::Scene & scene, const scene::Handle & options) {
            scene.set(options, "maxDepth", scene::Value::integer(1024));
            scene.set(options, "volumeDepth", scene::Value::integer(1024));
            scene.create("domeLight", "sky");
            if (fogged) {
                ball(scene, "fog", 1.0F, fogOf(scene, "fog", glm::vec3 { 0.0F }, glm::vec3 { 0.5F, 2.0F, 5.0F }, 0.5F), 0, turned());
            }
        });
    };
    const glm::vec3 bare = meanColour(furnace(false));
    const glm::vec3 fogged = meanColour(furnace(true));
    BOOST_TEST(near(fogged, bare, 0.02F), fogged.x << " " << fogged.y << " " << fogged.z << " against " << bare.x << " " << bare.y << " " << bare.z);
}

// a sphere of a medium lit by a sphere light gives the same picture within its errors however
// the light is found. Light samples alone, the phase function alone, and both weighed against
// each other agree
BOOST_AUTO_TEST_CASE(moya_media_light_and_phase_samples_find_the_same_light_test) {
    const auto lit = [](const char * strategy) {
        return pictured(256, [&](scene::Scene & scene, const scene::Handle & options) {
            scene.set(options, "lightSampling", scene::Value::string(strategy));
            const scene::Handle lamp = scene.create("sphereLight", "lamp");
            scene.set(lamp, "radius", scene::Value::number(0.5F));
            scene.set(lamp, "intensity", scene::Value::number(4.0F));
            scene.set(lamp, "transform", scene::Value::matrix(moved(1.5F, 1.0F, -1.0F)));
            ball(scene, "fog", 1.0F, fogOf(scene, "fog", glm::vec3 { 0.2F, 0.1F, 0.4F }, glm::vec3 { 0.6F, 1.5F, 3.0F }, 0.3F), 0, turned());
        });
    };
    const film::Estimate both = film::mean(lit("mis"), { 0, 0 }, { kSmall, kSmall });
    BOOST_TEST(both.value > 0.0F);
    for (const char * strategy : { "light", "bsdf" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Estimate alone = film::mean(lit(strategy), { 0, 0 }, { kSmall, kSmall });
            BOOST_TEST(film::sigmas(alone, both) < 4.0F, alone.value << " against " << both.value);
        }
    }
}

// a glass that is not thin-walled and has a transmission depth takes its transmission colour as
// the medium inside, over that depth, rather than at its surface. A glass of index one in front
// of a white dome lets through the colour to the power of its diameter over the depth
BOOST_AUTO_TEST_CASE(moya_media_a_deep_glass_absorbs_over_the_distance_inside_it_test) {
    const glm::vec3 tint { 0.8F, 0.5F, 0.2F };
    constexpr float kDepth = 1.5F;
    const film::Snapshot snapshot = pictured(256, [&](scene::Scene & scene, const scene::Handle &) {
        scene.create("domeLight", "sky");
        const scene::Handle tinted = materialOf(scene, "tinted", "surface",
            node(scene, "tinted", kOpenPbr,
                { { "transmission_weight", scene::Value::number(1.0F) }, { "specular_ior", scene::Value::number(1.0F) },
                    { "specular_roughness", scene::Value::number(0.0F) }, { "transmission_color", scene::Value::colour(tint) },
                    { "transmission_depth", scene::Value::number(kDepth) } }));
        ball(scene, "glass", 1.0F, tinted, 0, turned());
    });
    const glm::vec3 expected = colourOf([&](const spectrum::SampledWavelengths & wavelengths) {
        v3d::moya::kernel::surface::OpenPbr surface;
        surface.transmissionColour = tint;
        surface.transmissionDepth = kDepth;
        const volume::Coefficients coefficients = volume::coefficientsOf(colour::acescgTable(), volume::openpbr::interior(surface), wavelengths);
        return volume::transmittance(coefficients.extinction(), 2.0F) * spectrum::illuminant(colour::acescgTable(), glm::vec3 { 1.0F }, wavelengths);
    });
    BOOST_TEST(near(meanColour(snapshot), expected, 0.02F), meanColour(snapshot).x << " " << meanColour(snapshot).y << " " << meanColour(snapshot).z << " against "
        << expected.x << " " << expected.y << " " << expected.z);
}

// the options' volumeDepth limits a path's scattering in media, four times unless it is set
BOOST_AUTO_TEST_CASE(moya_media_the_options_limit_a_paths_scattering_in_media_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = aimed(scene, 1);
    scene.commit();
    scene.apply();
    BOOST_TEST(integrate::settingsOf(scene).volumeDepth == 4U);
    scene.set(options, "volumeDepth", scene::Value::integer(7));
    scene.commit();
    scene.apply();
    BOOST_TEST(integrate::settingsOf(scene).volumeDepth == 7U);
}

};  // namespace
};  // namespace v3d::moya::tracing
