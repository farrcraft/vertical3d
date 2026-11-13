/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace geometry = v3d::moya::geometry;
namespace integrate = v3d::moya::integrate;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";
constexpr const char * kStandard = "ND_standard_surface_surfaceshader";
constexpr const char * kUsd = "ND_UsdPreviewSurface_surfaceshader";
constexpr const char * kGltf = "ND_gltf_pbr_surfaceshader";

/** How near a translated parameter comes to the value it is expected to give. **/
constexpr float kNear = 1.0e-5F;

using Inputs = std::vector<std::pair<std::string, scene::Value>>;

/** A shader node of a definition with inputs. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const Inputs & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    for (const auto & [input, value] : inputs) {
        scene.declare(out, scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
        scene.set(out, input, value);
    }
    return out;
}

/**
 * A camera five units back along -z looking at a sphere of radius one at the origin, at 32 pixels
 * square and 64 samples per pixel, under a white dome light of no image. The sphere's material's
 * surface is a node of a definition with inputs.
 **/
void buildSphere(scene::Scene & scene, const std::string & definition, const Inputs & inputs) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ 32, 32 }));
    scene.set(options, "samples", scene::Value::integer(64));
    for (const char * depth : { "maxDepth", "glossyDepth", "specularDepth", "transmissionDepth" }) {
        scene.set(options, depth, scene::Value::integer(16));
    }
    scene.create("domeLight", "sky");
    scene.set(camera, "fov", scene::Value::number(30.0F));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    const scene::Handle material = scene.create("material", "tested");
    scene.set(material, "surface", scene::Value::handle(node(scene, "surface", definition, inputs)));
    const scene::Handle placed = instanced(scene, "ball", scene.create("sphere", "ball"), {}, glm::mat4(1.0F));
    scene.set(placed, "material", scene::Value::handle(material));
    scene.commit();
    scene.apply();
}

/** The material a surface node of a definition and inputs compiles to, and what was reported. **/
std::pair<kernel::surface::Material, std::vector<std::string>> compiled(const std::string & definition, const Inputs & inputs) {
    scene::Scene scene(scene::kinds());
    buildSphere(scene, definition, inputs);
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
    return { prototypes.materials.front(), prototypes.reports };
}

kernel::surface::OpenPbr parametersOf(const std::string & definition, const Inputs & inputs) {
    return compiled(definition, inputs).first.surface;
}

film::Snapshot sphereOf(const std::string & definition, const Inputs & inputs) {
    scene::Scene scene(scene::kinds());
    buildSphere(scene, definition, inputs);
    CpuDevice device;
    return integrate::render(device, scene, {}, v3d::moya::colour::Config());
}

/** Whether two colours are each within kNear of the other in every component. **/
bool near(const glm::vec3 & one, const glm::vec3 & other) {
    return std::fabs(one.x - other.x) < kNear && std::fabs(one.y - other.y) < kNear && std::fabs(one.z - other.z) < kNear;
}

// Standard Surface's defaults are OpenPBR's equivalent. At its version 1.0.1 they are a base of
// weight one, a base colour of 0.8 and a specular roughness of 0.2, neither cut out nor emitting
BOOST_AUTO_TEST_CASE(moya_shaders_standard_surface_defaults_translate_onto_openpbr_test) {
    const auto [material, reports] = compiled(kStandard, {});
    BOOST_TEST(material.layered == 1U);
    BOOST_TEST(material.surface.baseWeight == 1.0F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(near(material.surface.baseColour, glm::vec3 { 0.8F }));
    BOOST_TEST(material.surface.baseMetalness == 0.0F);
    BOOST_TEST(material.surface.specularWeight == 1.0F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(material.surface.specularRoughness == 0.2F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(material.surface.specularIor == 1.5F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(material.surface.coatWeight == 0.0F);
    BOOST_TEST(material.surface.emissionLuminance == 0.0F);
    BOOST_TEST(!material.cutOut());
}

// Standard Surface's emission survives its translation as the constants a light is made from, and
// so does its thin wall
BOOST_AUTO_TEST_CASE(moya_shaders_standard_surface_keeps_its_emission_and_thin_wall_test) {
    const auto [material, reports] = compiled(kStandard,
        { { "emission", scene::Value::number(2.0F) }, { "emission_color", scene::Value::colour(glm::vec3 { 1.0F, 0.5F, 0.25F }) },
            { "thin_walled", scene::Value::integer(1) } });
    BOOST_TEST(material.surface.emissionLuminance == 2.0F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(near(material.surface.emissionColour, glm::vec3 { 1.0F, 0.5F, 0.25F }));
    BOOST_TEST(material.surface.thinWalled == 1U);
}

// UsdPreviewSurface's defaults are a base colour of 0.18 and a specular roughness of 0.5, and its
// specular workflow reflects its specular colour at normal incidence
BOOST_AUTO_TEST_CASE(moya_shaders_usd_preview_surface_translates_onto_openpbr_test) {
    const kernel::surface::OpenPbr defaults = parametersOf(kUsd, {});
    BOOST_TEST(near(defaults.baseColour, glm::vec3 { 0.18F }));
    BOOST_TEST(defaults.baseMetalness == 0.0F);
    BOOST_TEST(defaults.specularRoughness == 0.5F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(defaults.specularIor == 1.5F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(near(defaults.specularColour, glm::vec3 { 1.0F }));
    BOOST_TEST(!compiled(kUsd, {}).first.cutOut());

    const kernel::surface::OpenPbr specular = parametersOf(kUsd,
        { { "useSpecularWorkflow", scene::Value::integer(1) }, { "specularColor", scene::Value::colour(glm::vec3 { 0.04F, 0.08F, 0.04F }) },
            { "metallic", scene::Value::number(1.0F) } });
    const float root = std::sqrt(0.08F);
    BOOST_TEST(specular.baseMetalness == 0.0F);
    BOOST_TEST(specular.specularIor == (1.0F + root) / (1.0F - root), boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(near(specular.specularColour, glm::vec3 { 0.5F, 1.0F, 0.5F }));

    const kernel::surface::Material masked = compiled(kUsd, { { "opacity", scene::Value::number(0.3F) }, { "opacityThreshold", scene::Value::number(0.5F) } }).first;
    BOOST_TEST(masked.cutOut());
    BOOST_TEST(masked.surface.geometryOpacity == 0.0F);
}

// glTF PBR's defaults are a white metal of roughness one, and its alpha modes cut it out by the
// cutoff, blend it by alpha, or leave it opaque
BOOST_AUTO_TEST_CASE(moya_shaders_gltf_pbr_translates_onto_openpbr_test) {
    const kernel::surface::Material defaults = compiled(kGltf, {}).first;
    BOOST_TEST(near(defaults.surface.baseColour, glm::vec3 { 1.0F }));
    BOOST_TEST(defaults.surface.baseMetalness == 1.0F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(defaults.surface.specularRoughness == 1.0F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(defaults.surface.fuzzWeight == 0.0F);
    BOOST_TEST(defaults.surface.thinFilmThickness == 0.1F, boost::test_tools::tolerance(kNear));
    BOOST_TEST(!defaults.cutOut());

    const Inputs translucent { { "alpha", scene::Value::number(0.3F) } };
    BOOST_TEST(!compiled(kGltf, translucent).first.cutOut());
    const kernel::surface::Material masked = compiled(kGltf, { { "alpha", scene::Value::number(0.3F) }, { "alpha_mode", scene::Value::integer(1) } }).first;
    BOOST_TEST(masked.surface.geometryOpacity == 0.0F);
    const kernel::surface::Material blended = compiled(kGltf, { { "alpha", scene::Value::number(0.3F) }, { "alpha_mode", scene::Value::integer(2) } }).first;
    BOOST_TEST(blended.surface.geometryOpacity == 0.3F, boost::test_tools::tolerance(kNear));

    const kernel::surface::OpenPbr sheened = parametersOf(kGltf, { { "sheen_color", scene::Value::colour(glm::vec3 { 0.5F, 0.2F, 0.1F }) } });
    BOOST_TEST(sheened.fuzzWeight == 1.0F);
    BOOST_TEST(near(sheened.fuzzColour, glm::vec3 { 0.5F, 0.2F, 0.1F }));
}

// a metal and a glass of each shader render as the OpenPBR material they translate onto does,
// within their errors. UsdPreviewSurface has no transmission, so it makes no glass
BOOST_AUTO_TEST_CASE(moya_shaders_a_metal_and_a_glass_render_as_openpbrs_test) {
    const scene::Value gold = scene::Value::colour(glm::vec3 { 0.9F, 0.6F, 0.2F });
    const film::Snapshot metal =
        sphereOf(kOpenPbr, { { "base_color", gold }, { "base_metalness", scene::Value::number(1.0F) }, { "specular_roughness", scene::Value::number(0.3F) } });
    const film::Snapshot glass = sphereOf(kOpenPbr, { { "transmission_weight", scene::Value::number(1.0F) }, { "specular_roughness", scene::Value::number(0.0F) } });
    const std::vector<std::pair<std::string, film::Snapshot>> metals {
        { "standard surface",
            sphereOf(kStandard,
                { { "base", scene::Value::number(1.0F) }, { "base_color", gold }, { "metalness", scene::Value::number(1.0F) },
                    { "specular_roughness", scene::Value::number(0.3F) } }) },
        { "usd preview surface",
            sphereOf(kUsd, { { "diffuseColor", gold }, { "metallic", scene::Value::number(1.0F) }, { "roughness", scene::Value::number(0.3F) } }) },
        { "gltf pbr", sphereOf(kGltf, { { "base_color", gold }, { "metallic", scene::Value::number(1.0F) }, { "roughness", scene::Value::number(0.3F) } }) },
    };
    for (const auto & [name, snapshot] : metals) {
        BOOST_TEST_CONTEXT(name << " metal") {
            const film::Furthest furthest = film::furthest(metal, snapshot);
            BOOST_TEST(furthest.sigmas < 5.0F);
        }
    }
    const std::vector<std::pair<std::string, film::Snapshot>> glasses {
        { "standard surface",
            sphereOf(kStandard,
                { { "base", scene::Value::number(1.0F) }, { "transmission", scene::Value::number(1.0F) }, { "specular_roughness", scene::Value::number(0.0F) } }) },
        { "gltf pbr",
            sphereOf(kGltf,
                { { "metallic", scene::Value::number(0.0F) }, { "transmission", scene::Value::number(1.0F) }, { "roughness", scene::Value::number(0.0F) } }) },
    };
    for (const auto & [name, snapshot] : glasses) {
        BOOST_TEST_CONTEXT(name << " glass") {
            const film::Furthest furthest = film::furthest(glass, snapshot);
            BOOST_TEST(furthest.sigmas < 5.0F);
        }
    }
    // the comparison tells the metal from the glass
    BOOST_TEST(film::furthest(metal, glass).sigmas > 5.0F);
}

};  // namespace

};  // namespace v3d::moya::tracing
