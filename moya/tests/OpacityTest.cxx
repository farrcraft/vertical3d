/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfPixelType.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace geometry = v3d::moya::geometry;
namespace integrate = v3d::moya::integrate;
namespace scene = v3d::moya::scene;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 32;

/** The pixels well inside the picture: the first along each axis, and the one past the last. **/
constexpr std::uint32_t kInsideFrom = 4;
constexpr std::uint32_t kInsideTo = 28;

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

void connect(scene::Scene & scene, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", std::to_string(from.index) + " to " + input + " of " + std::to_string(to.index));
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** A black OpenPBR surface that reflects nothing, of an opacity, as a node a material can name. **/
scene::Handle blackSurface(scene::Scene & scene, const std::string & name, float opacity) {
    return node(scene, name, kOpenPbr,
        { { "base_color", scene::Value::colour(glm::vec3 { 0.0F }) }, { "specular_weight", scene::Value::number(0.0F) },
            { "geometry_opacity", scene::Value::number(opacity) } });
}

scene::Handle materialOf(scene::Scene & scene, const std::string & name, const scene::Handle & surface) {
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "surface", scene::Value::handle(surface));
    return out;
}

/**
 * A camera five units back along -z looking at the origin, at kSide pixels square, a field of
 * view of thirty degrees and samples per pixel, under a white dome light of no image.
 **/
scene::Handle framed(scene::Scene & scene, std::uint32_t samples) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    scene.create("domeLight", "sky");
    scene.set(camera, "fov", scene::Value::number(30.0F));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    return options;
}

/**
 * A square of a half-side in the plane of z at a depth, facing the camera, whose st runs from zero
 * at its bottom left to one at its top right. A grey, where one is given, is its Cs.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the half-side, then the depth
scene::Handle square(scene::Scene & scene, const std::string & name, float half, float depth, float grey = -1.0F) {
    const scene::Handle out = scene.create("mesh", name);
    scene.set(out, "P", scene::Value::points({ { -half, -half, depth }, { half, -half, depth }, { half, half, depth }, { -half, half, depth } }));
    scene.set(out, "vertices", scene::Value::integers({ 3, 3 }));
    scene.set(out, "indices", scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
    const scene::Value st = scene::Value::floats({ 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F });
    scene.declare(out,
        scene::Declaration { .name = "st", .type = st.type(), .count = scene::Declaration::kAny, .width = 2, .fallback = st, .interpolation = scene::Interpolation::Vertex });
    scene.set(out, "st", st);
    if (grey >= 0.0F) {
        scene.declare(out, scene::Declaration { .name = "Cs",
            .type = scene::Type::Colour,
            .count = scene::Declaration::kAny,
            .fallback = scene::Value::colours({}),
            .interpolation = scene::Interpolation::Constant });
        scene.set(out, "Cs", scene::Value::colours({ glm::vec3 { grey } }));
    }
    return out;
}

/** A card placed with a material, hidden from camera rays when it is to cast shadows alone. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then what it places
scene::Handle card(scene::Scene & scene, const std::string & name, const scene::Handle & mesh, const scene::Handle & material, bool seen) {
    const scene::Handle out = instanced(scene, name, mesh, {}, glm::mat4(1.0F));
    scene.set(out, "material", scene::Value::handle(material));
    scene.set(out, "camera", scene::Value::integer(seen ? 1 : 0));
    return out;
}

/**
 * A white floor filling the picture at z = 0, under a black card of a material just in front of
 * it that the camera does not see, so the card's shadow falls on the floor.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the strategy, then the card's half-side
void buildShaded(scene::Scene & scene, const std::string & strategy, const scene::Handle & material, float half) {
    const scene::Handle options = framed(scene, 64);
    scene.set(options, "lightSampling", scene::Value::string(strategy));
    instanced(scene, "floor", square(scene, "floor", 3.0F, 0.0F, 1.0F), {}, glm::mat4(1.0F));
    card(scene, "card", square(scene, "card", half, -0.05F), material, false);
    scene.commit();
    scene.apply();
}

/** Writes a scanline EXR file of one float channel, Y. **/
void writeY(const std::string & path, std::uint32_t width, std::uint32_t height, std::vector<float> values) {
    Imf::Header header(static_cast<int>(width), static_cast<int>(height));
    header.channels().insert("Y", Imf::Channel(Imf::FLOAT));
    Imf::FrameBuffer buffer;
    buffer.insert("Y", Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(values.data()), sizeof(float), sizeof(float) * width));
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(buffer);
    file.writePixels(static_cast<int>(height));
}

/** Whether two snapshots hold the same floats, bit for bit, and the same counts of samples. **/
bool sameBytes(const film::Snapshot & one, const film::Snapshot & other) {
    return one.values.size() == other.values.size() && one.samples == other.samples &&
        std::memcmp(one.values.data(), other.values.data(), one.values.size() * sizeof(float)) == 0;
}

// geometry_opacity compiles: a constant below one cuts the material out, a node that gives it the
// same everywhere folds into the constant it gives, and one that varies makes the program give it
BOOST_AUTO_TEST_CASE(moya_opacity_a_material_is_cut_out_by_its_opacity_test) {
    scene::Scene scene(scene::kinds());
    framed(scene, 1);
    const scene::Handle solid = materialOf(scene, "solid", blackSurface(scene, "solid", 1.0F));
    const scene::Handle half = materialOf(scene, "half", blackSurface(scene, "half", 0.25F));
    const scene::Handle fed = blackSurface(scene, "fed", 1.0F);
    connect(scene, node(scene, "level", "ND_constant_float", { { "value", scene::Value::number(0.5F) } }), fed, "geometry_opacity");
    const scene::Handle folded = materialOf(scene, "folded", fed);
    const scene::Handle varied = blackSurface(scene, "varied", 1.0F);
    const scene::Handle across = node(scene, "across", "ND_extract_vector2", { { "index", scene::Value::integer(0) } });
    connect(scene, node(scene, "st", "ND_texcoord_vector2", {}), across, "in");
    connect(scene, across, varied, "geometry_opacity");
    const scene::Handle programmed = materialOf(scene, "programmed", varied);
    float x = 0.0F;
    for (const auto & [name, material] : { std::pair { "solid", solid }, std::pair { "half", half }, std::pair { "folded", folded }, std::pair { "programmed", programmed } }) {
        card(scene, name, square(scene, name, 1.0F, x), material, true);
        x += 1.0F;
    }
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 4U);
    BOOST_TEST(!prototypes.materials[0].cutOut());
    BOOST_TEST(prototypes.materials[1].cutOut());
    BOOST_TEST(prototypes.materials[1].surface.geometryOpacity == 0.25F);
    BOOST_TEST(prototypes.materials[1].programmedOpacity == 0U);
    BOOST_TEST(prototypes.materials[2].cutOut());
    BOOST_TEST(prototypes.materials[2].programmedOpacity == 0U);
    BOOST_TEST(prototypes.materials[2].surface.geometryOpacity == 0.5F);
    BOOST_TEST(prototypes.materials[3].cutOut());
    BOOST_TEST(prototypes.materials[3].programmedOpacity == 1U);
}

// a card of no opacity in front of the camera leaves every byte of the picture as it was
BOOST_AUTO_TEST_CASE(moya_opacity_a_card_of_no_opacity_changes_nothing_test) {
    const auto render = [](bool carded) {
        scene::Scene scene(scene::kinds());
        framed(scene, 16);
        instanced(scene, "floor", square(scene, "floor", 3.0F, 0.0F, 0.6F), {}, glm::mat4(1.0F));
        if (carded) {
            card(scene, "card", square(scene, "card", 3.0F, -1.0F), materialOf(scene, "clear", blackSurface(scene, "clear", 0.0F)), true);
        }
        scene.commit();
        scene.apply();
        CpuDevice device;
        return integrate::render(device, scene, {}, v3d::moya::colour::Config());
    };
    BOOST_TEST(sameBytes(render(false), render(true)));
}

// the dome seen through a black card of half opacity shows half of its light
BOOST_AUTO_TEST_CASE(moya_opacity_a_half_opaque_card_shows_half_of_what_lies_behind_it_test) {
    scene::Scene scene(scene::kinds());
    framed(scene, 64);
    card(scene, "card", square(scene, "card", 3.0F, 0.0F), materialOf(scene, "half", blackSurface(scene, "half", 0.5F)), true);
    scene.commit();
    scene.apply();
    CpuDevice device;
    const film::Snapshot snapshot = integrate::render(device, scene, {}, v3d::moya::colour::Config());
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 0.5F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 0.6F) > 10.0F);
}

/** The mean alpha of the pixels from one corner to the other, the second left out. **/
float meanAlpha(const film::Snapshot & snapshot, std::uint32_t from, std::uint32_t to) {
    double sum = 0.0;
    for (std::uint32_t y = from; y < to; ++y) {
        for (std::uint32_t x = from; x < to; ++x) {
            sum += snapshot.at(x, y, 3);
        }
    }
    return static_cast<float>(sum / static_cast<double>((to - from) * (to - from)));
}

// a camera ray that meets a surface covers its sample, and one that leaves for the dome does not.
// The alpha is then one where every sample meets the square, and zero where none does
BOOST_AUTO_TEST_CASE(moya_opacity_alpha_is_the_share_of_samples_that_meet_something_test) {
    scene::Scene scene(scene::kinds());
    framed(scene, 16);
    card(scene, "card", square(scene, "card", 0.5F, 0.0F), materialOf(scene, "solid", blackSurface(scene, "solid", 1.0F)), true);
    scene.commit();
    scene.apply();
    CpuDevice device;
    const film::Snapshot snapshot = integrate::render(device, scene, {}, v3d::moya::colour::Config());
    BOOST_TEST(snapshot.at(kSide / 2, kSide / 2, 3) == 1.0F);
    BOOST_TEST(snapshot.at(0, 0, 3) == 0.0F);
    BOOST_TEST(snapshot.at(0, 0, 0) > 0.0F);
    // the edge pixels hold the square's share of each, so the alpha's mean over the picture is the
    // square's share of the screen. The camera keeps the orthographic projection the scene falls
    // back to, whose screen runs from -1 to 1, so a square of side one covers a quarter of it
    BOOST_TEST(meanAlpha(snapshot, 0, kSide) == 0.25F, boost::test_tools::tolerance(0.02F));
}

// a camera ray passes a card a quarter opaque three times in four, so the card's alpha is a
// quarter
BOOST_AUTO_TEST_CASE(moya_opacity_a_quarter_opaque_card_has_a_quarter_alpha_test) {
    scene::Scene scene(scene::kinds());
    framed(scene, 64);
    card(scene, "card", square(scene, "card", 3.0F, 0.0F), materialOf(scene, "quarter", blackSurface(scene, "quarter", 0.25F)), true);
    scene.commit();
    scene.apply();
    CpuDevice device;
    const film::Snapshot snapshot = integrate::render(device, scene, {}, v3d::moya::colour::Config());
    // 576 pixels of 64 samples each give a standard error near two thousandths
    BOOST_TEST(meanAlpha(snapshot, kInsideFrom, kInsideTo) == 0.25F, boost::test_tools::tolerance(0.05F));
}

// a black card of half opacity over a white floor lets half of the dome's light through, whether
// light samples meet it, rays drawn from the floor's BSDF do, or both under multiple importance
BOOST_AUTO_TEST_CASE(moya_opacity_a_half_opaque_card_lets_half_of_the_light_through_test) {
    for (const char * strategy : { "mis", "light", "bsdf" }) {
        BOOST_TEST_CONTEXT(strategy) {
            scene::Scene scene(scene::kinds());
            buildShaded(scene, strategy, materialOf(scene, "half", blackSurface(scene, "half", 0.5F)), 100.0F);
            CpuDevice device;
            const film::Snapshot snapshot = integrate::render(device, scene, {}, v3d::moya::colour::Config());
            BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 0.5F) < 3.0F);
            BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 0.6F) > 10.0F);
        }
    }
}

// a card whose opacity is an image, solid on one half and cut away on the other, casts the
// shadow of its solid half alone. The picture is the same on one thread and on every thread
BOOST_AUTO_TEST_CASE(moya_opacity_a_card_cut_out_by_a_texture_casts_the_shadow_of_what_is_left_test) {
    const boost::filesystem::path directory = boost::filesystem::temp_directory_path() / "v3d_opacity_leaf";
    boost::filesystem::create_directories(directory);
    const std::string file = (directory / "leaf.exr").string();
    constexpr std::uint32_t kTexels = 16;
    std::vector<float> leaf;
    for (std::uint32_t row = 0; row < kTexels; ++row) {
        for (std::uint32_t column = 0; column < kTexels; ++column) {
            leaf.push_back(column < kTexels / 2U ? 1.0F : 0.0F);
        }
    }
    writeY(file, kTexels, kTexels, leaf);
    const auto render = [&](unsigned int threads) {
        scene::Scene scene(scene::kinds());
        const scene::Handle surface = blackSurface(scene, "leaf", 1.0F);
        connect(scene, node(scene, "read", "ND_image_float", { { "file", scene::Value::string(file) } }), surface, "geometry_opacity");
        buildShaded(scene, "mis", materialOf(scene, "leaf", surface), 3.0F);
        CpuDevice device(threads);
        return integrate::render(device, scene, boost::make_shared<v3d::log::Logger>(), v3d::moya::colour::Config());
    };
    const film::Snapshot snapshot = render(0);
    const film::Snapshot single = render(1);
    boost::system::error_code ignored;
    boost::filesystem::remove_all(directory, ignored);
    // the halves meet at the middle column; each is measured clear of the edge
    const film::Estimate left = film::mean(snapshot, glm::uvec2 { 2, kInsideFrom }, glm::uvec2 { 12, kInsideTo });
    const film::Estimate right = film::mean(snapshot, glm::uvec2 { 20, kInsideFrom }, glm::uvec2 { 30, kInsideTo });
    BOOST_TEST(std::min(left.value, right.value) < 0.1F);
    BOOST_TEST(std::max(left.value, right.value) > 0.9F);
    BOOST_TEST(sameBytes(snapshot, single));
}

// a mesh that is cut out is built with no spatial splits, so a ray meets each of its triangles
// once. The same mesh solid is split, and holds more references than triangles
BOOST_AUTO_TEST_CASE(moya_opacity_a_cut_out_mesh_holds_each_triangle_once_test) {
    constexpr std::size_t kSlivers = 64;
    for (const float opacity : { 1.0F, 0.5F }) {
        BOOST_TEST_CONTEXT("opacity " << opacity) {
            scene::Scene scene(scene::kinds());
            const scene::Handle placed = instanced(scene, "slivers", mesh(scene, "slivers", {}, { slivers(kSlivers, 5.0F) }), {}, glm::mat4(1.0F));
            scene.set(placed, "material", scene::Value::handle(materialOf(scene, "card", blackSurface(scene, "card", opacity))));
            scene.commit();
            scene.apply();
            CpuDevice device;
            const v3d::moya::accel::Assembly assembly = v3d::moya::accel::assemble(device, geometry::gather(scene, {}));
            // the world's hierarchy holds the one instance
            const std::size_t references = assembly.references.size() - 1U;
            if (opacity < 1.0F) {
                BOOST_TEST(references == kSlivers);
            } else {
                BOOST_TEST(references > kSlivers);
            }
        }
    }
}

};  // namespace

};  // namespace v3d::moya::tracing
