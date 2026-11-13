/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfOutputFile.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace geometry = v3d::moya::geometry;
namespace scene = v3d::moya::scene;

/** An input of a node and its value. **/
struct Given {
    const char * name;
    scene::Value value;
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const std::vector<Given> & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    for (const Given & input : inputs) {
        scene.declare(out, scene::Declaration { .name = input.name, .type = input.value.type(), .count = static_cast<std::uint32_t>(input.value.size()), .fallback = input.value });
        scene.set(out, input.name, input.value);
    }
    return out;
}

void connect(scene::Scene & scene, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", std::to_string(from.index) + " to " + input + " of " + std::to_string(to.index));
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** A material whose displacement terminal is a node. **/
scene::Handle displacing(scene::Scene & scene, const std::string & name, const scene::Handle & displacement) {
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "displacement", scene::Value::handle(displacement));
    return out;
}

/**
 * A square subdivision surface of one face in z = 0 from -1 to 1, wound to face +z, whose `st`
 * is its x and y, so its tangent is +x. Its edges and corners are kept, so it is a surface and
 * not a hole. Its displacement bound is 2.
 **/
scene::Handle square(scene::Scene & scene) {
    const scene::Handle out = scene.create("subdivision", "square");
    scene.set(out, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(out, "vertices", scene::Value::integers({ 4 }));
    scene.set(out, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    const scene::Value st = scene::Value::floats({ -1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F, -1.0F, 1.0F });
    scene.declare(out, scene::Declaration { .name = "st",
        .type = st.type(),
        .count = scene::Declaration::kAny,
        .width = 2,
        .fallback = st,
        .interpolation = scene::Interpolation::Vertex });
    scene.set(out, "st", st);
    scene.set(out, "boundary", scene::Value::string("edges-and-corners"));
    scene.set(out, "displacementBound", scene::Value::number(2.0F));
    return out;
}

/**
 * The points the square is cut into, placed with a material, or with none where it is invalid.
 * Textures are made in a directory where one is given.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the surface, then its material
std::vector<glm::vec3> pointsOf(scene::Scene & scene, const scene::Handle & surface, const scene::Handle & material, const std::string & made = {}) {
    const scene::Handle options = scene.create("options", "options");
    scene.set(options, "edgeLength", scene::Value::number(0.5F));
    if (!made.empty()) {
        scene.set(options, "textureDirectory", scene::Value::string(made));
    }
    const scene::Handle placed = instanced(scene, "placed", surface, {}, glm::mat4(1.0F));
    if (material.valid()) {
        scene.set(placed, "material", scene::Value::handle(material));
    }
    scene.commit();
    scene.apply();
    return geometry::gather(scene, {}).points;
}

/** The points the square is cut into with no displacement. **/
std::vector<glm::vec3> undisplaced() {
    scene::Scene scene(scene::kinds());
    return pointsOf(scene, square(scene), {});
}

/** The largest distance between two lists of points of one length, each moved by an offset. **/
float apart(const std::vector<glm::vec3> & moved, const std::vector<glm::vec3> & from, const glm::vec3 & offset) {
    BOOST_TEST_REQUIRE(!from.empty());
    BOOST_TEST_REQUIRE(moved.size() == from.size());
    float out = 0.0F;
    for (std::size_t i = 0; i < moved.size(); ++i) {
        out = std::fmax(out, glm::distance(moved[i], from[i] + offset));
    }
    return out;
}

/** Writes a scanline EXR file of one float channel, Y, of one value. **/
void writeY(const std::string & path, std::uint32_t side, float value) {
    std::vector<float> values(static_cast<std::size_t>(side) * side, value);
    Imf::Header header(static_cast<int>(side), static_cast<int>(side));
    header.channels().insert("Y", Imf::Channel(Imf::FLOAT));
    Imf::FrameBuffer buffer;
    buffer.insert("Y", Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(values.data()), sizeof(float), sizeof(float) * side));
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(buffer);
    file.writePixels(static_cast<int>(side));
}

// a displacement of a float moves every point along the normal by its height times its scale
BOOST_AUTO_TEST_CASE(moya_displacement_a_float_moves_along_the_normal_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle raise =
        node(scene, "raise", "ND_displacement_float", { { "displacement", scene::Value::number(0.25F) }, { "scale", scene::Value::number(2.0F) } });
    BOOST_TEST(apart(pointsOf(scene, square(scene), displacing(scene, "raised", raise)), undisplaced(), glm::vec3 { 0.0F, 0.0F, 0.5F }) < 1.0e-5F);
}

// a displacement of a vector3 moves each point along the tangent, the bitangent and the normal
BOOST_AUTO_TEST_CASE(moya_displacement_a_vector_moves_along_each_axis_of_the_frame_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle shift = node(scene, "shift", "ND_displacement_vector3", { { "displacement", scene::Value::floats({ 0.1F, -0.2F, 0.3F }) } });
    BOOST_TEST(apart(pointsOf(scene, square(scene), displacing(scene, "shifted", shift)), undisplaced(), glm::vec3 { 0.1F, -0.2F, 0.3F }) < 1.0e-5F);
}

// a displacement that a node feeds is run at each point: a height of s raises each point by its x
BOOST_AUTO_TEST_CASE(moya_displacement_a_height_a_node_feeds_is_run_at_each_point_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle uv = node(scene, "uv", "ND_texcoord_vector2", {});
    const scene::Handle s = node(scene, "s", "ND_extract_vector2", { { "index", scene::Value::integer(0) } });
    connect(scene, uv, s, "in");
    const scene::Handle raise = node(scene, "raise", "ND_displacement_float", {});
    connect(scene, s, raise, "displacement");
    const std::vector<glm::vec3> moved = pointsOf(scene, square(scene), displacing(scene, "ramped", raise));
    const std::vector<glm::vec3> flat = undisplaced();
    BOOST_TEST_REQUIRE(!flat.empty());
    BOOST_TEST_REQUIRE(moved.size() == flat.size());
    float worst = 0.0F;
    for (std::size_t i = 0; i < moved.size(); ++i) {
        worst = std::fmax(worst, glm::distance(moved[i], flat[i] + glm::vec3 { 0.0F, 0.0F, flat[i].x }));
    }
    BOOST_TEST(worst < 1.0e-5F);
}

// a displacement read from an image moves each point by the texel it reads
BOOST_AUTO_TEST_CASE(moya_displacement_a_textured_height_moves_by_its_texels_test) {
    const boost::filesystem::path folder = boost::filesystem::temp_directory_path() / "v3d_displacement_textured";
    boost::system::error_code ignored;
    boost::filesystem::remove_all(folder, ignored);
    boost::filesystem::create_directories(folder);
    const std::string picture = (folder / "height.exr").string();
    writeY(picture, 8, 0.375F);
    scene::Scene scene(scene::kinds());
    const scene::Handle image = node(scene, "height", "ND_image_float", { { "file", scene::Value::string(picture) } });
    const scene::Handle raise = node(scene, "raise", "ND_displacement_float", {});
    connect(scene, image, raise, "displacement");
    const std::vector<glm::vec3> moved = pointsOf(scene, square(scene), displacing(scene, "textured", raise), (folder / "made").string());
    BOOST_TEST(apart(moved, undisplaced(), glm::vec3 { 0.0F, 0.0F, 0.375F }) < 1.0e-5F);
    boost::filesystem::remove_all(folder, ignored);
}

// a displacement longer than the bound is held to it along its own direction, and reported
BOOST_AUTO_TEST_CASE(moya_displacement_beyond_the_bound_is_held_to_it_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = square(scene);
    scene.set(surface, "displacementBound", scene::Value::number(0.5F));
    const scene::Handle shift = node(scene, "shift", "ND_displacement_vector3", { { "displacement", scene::Value::floats({ 0.0F, 0.6F, 0.8F }) } });
    const scene::Handle options = scene.create("options", "options");
    scene.set(options, "edgeLength", scene::Value::number(0.5F));
    const scene::Handle placed = instanced(scene, "placed", surface, {}, glm::mat4(1.0F));
    scene.set(placed, "material", scene::Value::handle(displacing(scene, "far", shift)));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "subdivision square is displaced beyond its bound of 0.5, and is held to it"));
    BOOST_TEST(apart(prototypes.points, undisplaced(), glm::vec3 { 0.0F, 0.3F, 0.4F }) < 1.0e-5F);
}

// a displacement of a node that is not a displacement shader is reported, and leaves the surface
// where its limit lies
BOOST_AUTO_TEST_CASE(moya_displacement_of_another_definition_is_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle shader = node(scene, "surface", "ND_open_pbr_surface_surfaceshader", {});
    const scene::Handle surface = square(scene);
    const scene::Handle options = scene.create("options", "options");
    scene.set(options, "edgeLength", scene::Value::number(0.5F));
    const scene::Handle placed = instanced(scene, "placed", surface, {}, glm::mat4(1.0F));
    scene.set(placed, "material", scene::Value::handle(displacing(scene, "unshaped", shader)));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "material unshaped's displacement is ND_open_pbr_surface_surfaceshader, which moya does not displace by, so it is not displaced"));
    BOOST_TEST(apart(prototypes.points, undisplaced(), glm::vec3 { 0.0F }) < 1.0e-6F);
}

// a surface placed twice is displaced by the material of the placement nearest the camera, which
// it is cut for
BOOST_AUTO_TEST_CASE(moya_displacement_comes_from_the_nearest_placement_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle surface = square(scene);
    const scene::Handle options = scene.create("options", "options");
    scene.set(options, "edgeLength", scene::Value::number(0.5F));
    const scene::Handle low = node(scene, "low", "ND_displacement_float", { { "displacement", scene::Value::number(0.1F) } });
    const scene::Handle high = node(scene, "high", "ND_displacement_float", { { "displacement", scene::Value::number(0.2F) } });
    const scene::Handle near = instanced(scene, "near", surface, {}, glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, 0.0F, 4.0F }));
    scene.set(near, "material", scene::Value::handle(displacing(scene, "near", high)));
    const scene::Handle far = instanced(scene, "far", surface, {}, glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, 0.0F, 40.0F }));
    scene.set(far, "material", scene::Value::handle(displacing(scene, "far", low)));
    scene.commit();
    scene.apply();
    const std::vector<glm::vec3> moved = geometry::gather(scene, {}).points;
    BOOST_TEST_REQUIRE(!moved.empty());
    float lowest = 1.0F;
    for (const glm::vec3 & point : moved) {
        lowest = std::fmin(lowest, point.z);
    }
    BOOST_TEST(lowest == 0.2F, boost::test_tools::tolerance(1.0e-5F));
}

};  // namespace
};  // namespace v3d::moya::tracing
