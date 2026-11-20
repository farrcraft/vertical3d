/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
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
namespace integrate = v3d::moya::integrate;
namespace scene = v3d::moya::scene;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The picture's side, in pixels, and the pixels from one corner to the other of its middle. **/
constexpr std::int32_t kSide = 16;
constexpr std::uint32_t kMiddleFrom = 6;
constexpr std::uint32_t kMiddleTo = 10;

/** The red wall's colour. **/
constexpr glm::vec3 kRed { 0.8F, 0.1F, 0.1F };

/**
 * A scene of a camera five units back along -z under a dome, whose film holds a denoiser's albedo
 * and normal. The camera keeps the orthographic projection the scene falls back to, whose screen
 * runs from -1 to 1.
 **/
struct Featured {
    scene::Scene made { scene::kinds() };

    Featured() {
        const scene::Handle options = made.create("options", "options");
        const scene::Handle camera = made.create("camera", "camera");
        made.set(options, "camera", scene::Value::handle(camera));
        made.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
        made.set(options, "samples", scene::Value::integer(64));
        made.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
        made.create("domeLight", "sky");
        const scene::Handle albedo = channel("albedo", "albedo");
        const scene::Handle normal = channel("normal", "denoisingNormal");
        const scene::Handle output = made.create("output", "picture");
        made.set(output, "file", scene::Value::string("picture.exr"));
        made.set(output, "channels", scene::Value::handles({ albedo, normal }));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then what it collects
    scene::Handle channel(const std::string & name, const std::string & source) {
        const scene::Handle out = made.create("channel", name);
        made.set(out, "sourceType", scene::Value::string("raw"));
        made.set(out, "sourceName", scene::Value::string(source));
        made.set(out, "components", scene::Value::integer(3));
        return out;
    }

    /** A material of an OpenPBR surface with the inputs given. **/
    scene::Handle material(const std::string & name, const std::vector<std::pair<std::string, scene::Value>> & inputs) {
        const scene::Handle surface = made.create("shader", name);
        made.set(surface, "shader", scene::Value::string(kOpenPbr));
        for (const auto & [input, value] : inputs) {
            made.declare(surface, scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
            made.set(surface, input, value);
        }
        const scene::Handle out = made.create("material", name);
        made.set(out, "surface", scene::Value::handle(surface));
        return out;
    }

    /** A quadrilateral of four corners, in their order, placed with a material. **/
    void quad(const std::string & name, const std::vector<glm::vec3> & corners, const scene::Handle & material) {
        const scene::Handle mesh = made.create("mesh", name);
        made.set(mesh, "P", scene::Value::points(corners));
        made.set(mesh, "vertices", scene::Value::integers({ 3, 3 }));
        made.set(mesh, "indices", scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
        const scene::Handle placed = instanced(made, name, mesh, {}, glm::mat4(1.0F));
        made.set(placed, "material", scene::Value::handle(material));
    }

    film::Snapshot render() {
        made.commit();
        made.apply();
        v3d::moya::device::CpuDevice device;
        return integrate::render(device, made, {}, colour::Config());
    }
};

/** A channel's mean over the picture's middle, component by component. **/
glm::vec3 middleOf(const film::Snapshot & picture, const std::string & name) {
    glm::vec3 sum { 0.0F };
    for (std::uint32_t y = kMiddleFrom; y < kMiddleTo; ++y) {
        for (std::uint32_t x = kMiddleFrom; x < kMiddleTo; ++x) {
            for (std::uint32_t i = 0; i < 3U; ++i) {
                sum[static_cast<glm::length_t>(i)] += picture.at(x, y, picture.offset(name) + i);
            }
        }
    }
    return sum / static_cast<float>((kMiddleTo - kMiddleFrom) * (kMiddleTo - kMiddleFrom));
}

// a grey diffuse square facing the camera gives its grey as its albedo and faces the camera. A
// pixel that sees only the dome holds zero in each
BOOST_AUTO_TEST_CASE(moya_denoisefeatures_a_diffuse_square_gives_its_colour_test) {
    Featured featured;
    const scene::Handle grey = featured.material("grey", { { "base_color", scene::Value::colour(glm::vec3 { 0.5F }) }, { "specular_weight", scene::Value::number(0.0F) } });
    featured.quad("square", { { -0.5F, -0.5F, 0.0F }, { 0.5F, -0.5F, 0.0F }, { 0.5F, 0.5F, 0.0F }, { -0.5F, 0.5F, 0.0F } }, grey);
    const film::Snapshot picture = featured.render();
    const glm::vec3 albedo = middleOf(picture, "albedo");
    for (glm::length_t i = 0; i < 3; ++i) {
        BOOST_TEST(albedo[i] == 0.5F, boost::test_tools::tolerance(0.02F));
    }
    BOOST_TEST(std::fabs(middleOf(picture, "normal").z) == 1.0F, boost::test_tools::tolerance(1.0e-4F));
    for (std::uint32_t i = 0; i < 3U; ++i) {
        BOOST_TEST(picture.at(0, 0, picture.offset("albedo") + i) == 0.0F);
        BOOST_TEST(picture.at(0, 0, picture.offset("normal") + i) == 0.0F);
    }
}

// a smooth white mirror turned 45 degrees about y sends each camera ray along +x to a red wall
// out of the camera's sight. A denoiser is given the wall's albedo, as the same red seen face on
// gives it, and the wall's normal, which lies along x, not the mirror's
BOOST_AUTO_TEST_CASE(moya_denoisefeatures_a_mirror_gives_the_albedo_of_what_it_shows_test) {
    Featured direct;
    const scene::Handle seen = direct.material("red", { { "base_color", scene::Value::colour(kRed) }, { "specular_weight", scene::Value::number(0.0F) } });
    direct.quad("wall", { { -3.0F, -3.0F, 0.0F }, { 3.0F, -3.0F, 0.0F }, { 3.0F, 3.0F, 0.0F }, { -3.0F, 3.0F, 0.0F } }, seen);
    const glm::vec3 want = middleOf(direct.render(), "albedo");

    Featured featured;
    const scene::Handle mirror = featured.material("mirror", { { "base_color", scene::Value::colour(glm::vec3 { 1.0F }) },
                                                                  { "base_metalness", scene::Value::number(1.0F) },
                                                                  { "specular_roughness", scene::Value::number(0.0F) } });
    const scene::Handle red = featured.material("red", { { "base_color", scene::Value::colour(kRed) }, { "specular_weight", scene::Value::number(0.0F) } });
    featured.quad("mirror", { { -0.7F, -0.7F, -0.7F }, { 0.7F, -0.7F, 0.7F }, { 0.7F, 0.7F, 0.7F }, { -0.7F, 0.7F, -0.7F } }, mirror);
    featured.quad("wall", { { 2.0F, -3.0F, -3.0F }, { 2.0F, 3.0F, -3.0F }, { 2.0F, 3.0F, 3.0F }, { 2.0F, -3.0F, 3.0F } }, red);
    const film::Snapshot picture = featured.render();
    const glm::vec3 albedo = middleOf(picture, "albedo");
    BOOST_TEST(want.x > 4.0F * want.y);
    for (glm::length_t i = 0; i < 3; ++i) {
        BOOST_TEST(albedo[i] == want[i], boost::test_tools::tolerance(0.02F));
    }
    const glm::vec3 normal = middleOf(picture, "normal");
    BOOST_TEST(std::fabs(normal.x) == 1.0F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(normal.z == 0.0F, boost::test_tools::tolerance(1.0e-4F));
}

};  // namespace
};  // namespace v3d::moya::tracing
