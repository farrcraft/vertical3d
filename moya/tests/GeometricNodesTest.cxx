/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembler.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
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
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
using kernel::surface::OpenPbr;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/** The float inputs of open_pbr_surface a test reads values through, in order. **/
constexpr const char * kReadThrough[6] = { "base_weight", "base_diffuse_roughness", "base_metalness", "specular_weight", "specular_roughness", "specular_roughness_anisotropy" };

/** An input of a node and its value. **/
struct Given {
    const char * name;
    scene::Value value;
};

/** Where a ray meets a scene's surface: its shading point, frame and program's parameters. **/
struct Shaded {
    kernel::surface::ShadingPoint point;
    kernel::surface::Frame frame;
    OpenPbr layers;
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

/**
 * Feeds the components of a node's output, of a MaterialX type, into the surface's float inputs
 * from the first given, one input for each component.
 **/
void readInto(scene::Scene & scene, const scene::Handle & from, const std::string & type, std::size_t components, const scene::Handle & surface, std::size_t first) {
    if (type == "float") {
        connect(scene, from, surface, kReadThrough[first]);
        return;
    }
    for (std::size_t i = 0; i < components; ++i) {
        const scene::Handle extract = node(scene, "extract " + std::to_string(from.index) + " " + std::to_string(i), "ND_extract_" + type,
            { { "index", scene::Value::integer(static_cast<std::int32_t>(i)) } });
        connect(scene, from, extract, "in");
        connect(scene, extract, surface, kReadThrough[first + i]);
    }
}

/** The values the surface's float inputs read through hold, in order. **/
std::vector<float> readBack(const OpenPbr & layers) {
    return { layers.baseWeight, layers.baseDiffuseRoughness, layers.baseMetalness, layers.specularWeight, layers.specularRoughness, layers.specularAnisotropy };
}

/** Declares a primvar on a geometry of a class, each of its elements of a width. **/
void primvar(scene::Scene & scene, const scene::Handle & geometry, const std::string & name, scene::Interpolation interpolation, const scene::Value & values,
    std::uint32_t width) {
    scene.declare(geometry, scene::Declaration { .name = name,
        .type = values.type(),
        .count = scene::Declaration::kAny,
        .width = width,
        .fallback = values,
        .interpolation = interpolation });
    scene.set(geometry, name, values);
}

/** A square of two faces in z = 0 from -1 to 1, facing +z. **/
scene::Handle square(scene::Scene & scene) {
    const scene::Handle out = scene.create("mesh", "square");
    scene.set(out, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(out, "vertices", scene::Value::integers({ 3, 3 }));
    scene.set(out, "indices", scene::Value::integers({ 0, 1, 2, 0, 2, 3 }));
    return out;
}

/** Places a geometry with a material whose surface is given, by a transformation. **/
void place(scene::Scene & scene, const scene::Handle & geometry, const scene::Handle & surface, const glm::mat4 & transform) {
    const scene::Handle held = scene.create("material", "tested");
    scene.set(held, "surface", scene::Value::handle(surface));
    const scene::Handle placed = scene.create("instance", "placed");
    scene.set(placed, "prototype", scene::Value::handle(geometry));
    scene.set(placed, "material", scene::Value::handle(held));
    scene.set(placed, "transform", scene::Value::matrix(transform));
    scene.commit();
    scene.apply();
}

Shaded shadedAt(const scene::Scene & scene, const kernel::geometry::Ray & ray) {
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    v3d::moya::device::CpuDevice device;
    const view::Traced traced = view::trace(device, prototypes, { ray }, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    Shaded out;
    out.point = held.at(traced.nearest.front(), ray);
    BOOST_TEST_REQUIRE(out.point.found());
    out.frame = held.tangentFrame(out.point);
    out.layers = kernel::program::evaluate(held, out.point, held.materials[out.point.material].program);
    return out;
}

/** A ray down onto the square at a point of it. **/
kernel::geometry::Ray downAt(float x, float y) {
    kernel::geometry::Ray out;
    out.origin = glm::vec3 { x, y, 3.0F };
    out.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    return out;
}

// a primvar of each class and of one to four floats reaches the program at a shading point, read
// between its corners, and one the geometry lacks gives its default
BOOST_AUTO_TEST_CASE(moya_geometric_a_primvar_of_each_class_and_width_reads_back_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = square(scene);
    primvar(scene, mesh, "weight", scene::Interpolation::Constant, scene::Value::floats({ 0.3F }), 1);
    primvar(scene, mesh, "pair", scene::Interpolation::Uniform, scene::Value::floats({ 0.1F, 0.2F, 0.4F, 0.5F }), 2);
    // each point's value is its position, and each face-vertex's its y, so a read is the hit's own
    primvar(scene, mesh, "triple", scene::Interpolation::Vertex,
        scene::Value::floats({ -1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F }), 3);
    primvar(scene, mesh, "quad", scene::Interpolation::FaceVarying,
        scene::Value::floats({ 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 0, 0, 1 }), 4);
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    readInto(scene, node(scene, "weight", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("weight") } }), "float", 1, surface, 0);
    readInto(scene, node(scene, "pair", "ND_geompropvalue_vector2", { { "geomprop", scene::Value::string("pair") } }), "vector2", 2, surface, 1);
    readInto(scene, node(scene, "triple", "ND_geompropvalue_vector3", { { "geomprop", scene::Value::string("triple") } }), "vector3", 1, surface, 3);
    const scene::Handle quad = node(scene, "quad", "ND_geompropvalue_vector4", { { "geomprop", scene::Value::string("quad") } });
    const scene::Handle fourth = node(scene, "fourth", "ND_extract_vector4", { { "index", scene::Value::integer(3) } });
    connect(scene, quad, fourth, "in");
    connect(scene, fourth, surface, kReadThrough[4]);
    readInto(scene, node(scene, "absent", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("absent") }, { "default", scene::Value::number(0.7F) } }),
        "float", 1, surface, 5);
    place(scene, mesh, surface, glm::mat4(1.0F));

    // the second face, the upper left half
    const std::vector<float> read = readBack(shadedAt(scene, downAt(-0.5F, 0.25F)).layers);
    BOOST_TEST(read[0] == 0.3F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(read[1] == 0.4F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(read[2] == 0.5F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(read[3] == -0.5F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(read[4] == 0.25F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(read[5] == 0.7F, boost::test_tools::tolerance(1.0e-6F));
}

// the tangent lies along the direction s increases, and the bitangent towards t's increase
BOOST_AUTO_TEST_CASE(moya_geometric_a_tangent_lies_along_the_direction_s_increases_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = square(scene);
    // s runs along +y and t along -x
    primvar(scene, mesh, "st", scene::Interpolation::Vertex, scene::Value::floats({ -1.0F, 1.0F, -1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F }), 2);
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    readInto(scene, node(scene, "uv", "ND_texcoord_vector2", {}), "vector2", 2, surface, 0);
    readInto(scene, node(scene, "along", "ND_tangent_vector3", { { "space", scene::Value::string("world") } }), "vector3", 3, surface, 2);
    place(scene, mesh, surface, glm::mat4(1.0F));

    const Shaded shaded = shadedAt(scene, downAt(0.3F, -0.4F));
    BOOST_TEST(shaded.frame.tangent.y == 1.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(shaded.frame.bitangent.x == -1.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(shaded.frame.normal.z == 1.0F, boost::test_tools::tolerance(1.0e-5F));
    const std::vector<float> read = readBack(shaded.layers);
    BOOST_TEST(read[0] == -0.4F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(read[1] == -0.3F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(read[2] == 0.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(read[3] == 1.0F, boost::test_tools::tolerance(1.0e-5F));
}

// mirrored texture coordinates turn the bitangent to the side t increases on
BOOST_AUTO_TEST_CASE(moya_geometric_a_mirrored_texture_turns_the_bitangent_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = square(scene);
    // s runs along +y and t along +x, so the normal's cross with the tangent faces away from t
    primvar(scene, mesh, "st", scene::Interpolation::Vertex, scene::Value::floats({ -1.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F, 1.0F, -1.0F }), 2);
    place(scene, mesh, node(scene, "surface", kOpenPbr, {}), glm::mat4(1.0F));

    const kernel::surface::Frame frame = shadedAt(scene, downAt(0.3F, -0.4F)).frame;
    BOOST_TEST(frame.tangent.y == 1.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(frame.bitangent.x == 1.0F, boost::test_tools::tolerance(1.0e-5F));
}

// a tangent primvar gives the tangent, turned into the world and laid flat against the normal
BOOST_AUTO_TEST_CASE(moya_geometric_a_tangent_primvar_gives_the_tangent_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = square(scene);
    primvar(scene, mesh, "tangent", scene::Interpolation::Constant, scene::Value::vectors({ { 1.0F, 1.0F, 0.5F } }), 1);
    place(scene, mesh, node(scene, "surface", kOpenPbr, {}), glm::rotate(glm::mat4(1.0F), glm::radians(90.0F), glm::vec3 { 0.0F, 0.0F, 1.0F }));

    const kernel::surface::Frame frame = shadedAt(scene, downAt(0.1F, 0.2F)).frame;
    const glm::vec3 expected = glm::normalize(glm::vec3 { -1.0F, 1.0F, 0.0F });
    BOOST_TEST(frame.tangent.x == expected.x, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(frame.tangent.y == expected.y, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(frame.tangent.z == 0.0F, boost::test_tools::tolerance(1.0e-5F));
}

// position and normal are given in the geometry's own space or in the world, as the node asks
BOOST_AUTO_TEST_CASE(moya_geometric_position_and_normal_are_in_the_space_asked_for_test) {
    // the square turned to face +x, and moved along y
    const glm::mat4 transform = glm::rotate(glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, 2.0F, 0.0F }), glm::radians(90.0F), glm::vec3 { 0.0F, 1.0F, 0.0F });
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 3.0F, 2.2F, -0.1F };
    ray.direction = glm::vec3 { -1.0F, 0.0F, 0.0F };

    scene::Scene positions(scene::kinds());
    const scene::Handle atPosition = node(positions, "surface", kOpenPbr, {});
    readInto(positions, node(positions, "world", "ND_position_vector3", { { "space", scene::Value::string("world") } }), "vector3", 3, atPosition, 0);
    readInto(positions, node(positions, "own", "ND_position_vector3", { { "space", scene::Value::string("object") } }), "vector3", 3, atPosition, 3);
    place(positions, square(positions), atPosition, transform);
    const std::vector<float> where = readBack(shadedAt(positions, ray).layers);
    const std::vector<float> expectedWhere { 0.0F, 2.2F, -0.1F, 0.1F, 0.2F, 0.0F };
    for (std::size_t i = 0; i < where.size(); ++i) {
        BOOST_TEST(where[i] == expectedWhere[i], boost::test_tools::tolerance(1.0e-5F));
    }

    scene::Scene normals(scene::kinds());
    const scene::Handle atNormal = node(normals, "surface", kOpenPbr, {});
    readInto(normals, node(normals, "world", "ND_normal_vector3", { { "space", scene::Value::string("world") } }), "vector3", 3, atNormal, 0);
    readInto(normals, node(normals, "own", "ND_normal_vector3", {}), "vector3", 3, atNormal, 3);
    place(normals, square(normals), atNormal, transform);
    const std::vector<float> facing = readBack(shadedAt(normals, ray).layers);
    const std::vector<float> expectedFacing { 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F };
    for (std::size_t i = 0; i < facing.size(); ++i) {
        BOOST_TEST(facing[i] == expectedFacing[i], boost::test_tools::tolerance(1.0e-5F));
    }
}

// a geometry the cache holds gains a primvar when a material starts to read it
BOOST_AUTO_TEST_CASE(moya_geometric_a_cached_geometry_gains_a_primvar_a_material_reads_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle mesh = square(scene);
    primvar(scene, mesh, "weight", scene::Interpolation::Constant, scene::Value::floats({ 0.3F }), 1);
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    place(scene, mesh, surface, glm::mat4(1.0F));
    v3d::moya::device::CpuDevice device;
    v3d::moya::accel::Assembler assembler(device, {});
    assembler.update(scene);
    const auto holds = [&](const std::string & name) {
        const geometry::Prototypes & prototypes = assembler.prototypes();
        const auto found = std::find(prototypes.primvarNames.begin(), prototypes.primvarNames.end(), name);
        if (found == prototypes.primvarNames.end()) {
            return false;
        }
        const auto number = static_cast<std::uint32_t>(found - prototypes.primvarNames.begin());
        return std::any_of(prototypes.primvars.begin(), prototypes.primvars.end(), [&](const kernel::surface::Primvar & each) {
            return each.name == number;
        });
    };
    BOOST_TEST(!holds("weight"));

    connect(scene, node(scene, "weight", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("weight") } }), surface, "base_weight");
    scene.commit();
    scene.apply();
    assembler.update(scene);
    BOOST_TEST(holds("weight"));
}

};  // namespace
