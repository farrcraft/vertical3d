/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/integrate/Media.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace film = v3d::moya::film;
namespace geometry = v3d::moya::geometry;
namespace integrate = v3d::moya::integrate;
namespace media = v3d::moya::kernel::integrate;
namespace scene = v3d::moya::scene;
namespace spectrum = v3d::moya::kernel::spectrum;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 24;

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
 * A closed sphere of a radius at the origin, placed with a material and a priority. Its cuts are
 * set to its poles, since RI's cut a sphere at one unit from its centre.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the radius
scene::Handle ball(scene::Scene & scene, const std::string & name, float radius, const scene::Handle & material, std::int32_t priority) {
    const scene::Handle sphere = scene.create("sphere", name);
    scene.set(sphere, "radius", scene::Value::number(radius));
    scene.set(sphere, "zmin", scene::Value::number(-radius));
    scene.set(sphere, "zmax", scene::Value::number(radius));
    const scene::Handle out = instanced(scene, name, sphere, {}, glm::mat4(1.0F));
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

};  // namespace
};  // namespace v3d::moya::tracing
