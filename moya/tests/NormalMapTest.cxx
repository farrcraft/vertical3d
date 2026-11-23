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
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Footprint.h>
#include <moya/libmoya/kernel/surface/Normal.h>
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
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

namespace bsdf = v3d::moya::kernel::bsdf;
namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

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

/**
 * A square of two faces in z = 0 from -1 to 1, facing +z, whose `st` is its x and y, so its
 * tangent is +x and its bitangent +y.
 **/
scene::Handle square(scene::Scene & scene) {
    const scene::Handle out = scene.create("mesh", "square");
    scene.set(out, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(out, "vertices", scene::Value::integers({ 3, 3 }));
    scene.set(out, "indices", scene::Value::integers({ 0, 1, 2, 0, 2, 3 }));
    const scene::Value st = scene::Value::floats({ -1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F, -1.0F, 1.0F });
    scene.declare(out, scene::Declaration { .name = "st",
        .type = st.type(),
        .count = scene::Declaration::kAny,
        .width = 2,
        .fallback = st,
        .interpolation = scene::Interpolation::Vertex });
    scene.set(out, "st", st);
    return out;
}

/** Places the square with a material whose surface is given. **/
void place(scene::Scene & scene, const scene::Handle & surface) {
    const scene::Handle held = scene.create("material", "tested");
    scene.set(held, "surface", scene::Value::handle(surface));
    const scene::Handle placed = scene.create("instance", "placed");
    scene.set(placed, "prototype", scene::Value::handle(square(scene)));
    scene.set(placed, "material", scene::Value::handle(held));
    scene.set(placed, "transform", scene::Value::matrix(glm::mat4(1.0F)));
    scene.commit();
    scene.apply();
}

/**
 * The BSDF where a ray straight down meets the square at (0.3, -0.4), for light leaving along
 * outgoing. A footprint, where one is given, is the one a cone of that width makes meeting the
 * square at an angle.
 **/
bsdf::Bsdf scatteringAt(const scene::Scene & scene, const glm::vec3 & outgoing, std::optional<float> footprint = std::nullopt) {
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    v3d::moya::device::CpuDevice device;
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 0.3F, -0.4F, 3.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    const view::Traced traced = view::trace(device, prototypes, { ray }, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    kernel::surface::ShadingPoint point = held.at(traced.nearest.front(), ray);
    BOOST_TEST_REQUIRE(point.found());
    if (footprint) {
        point.footprint = kernel::surface::footprintOf(*footprint, glm::normalize(glm::vec3 { 0.3F, 0.2F, -1.0F }), point.shading);
    }
    return bsdf::atSurface(held, point, colour::acescgTable(), v3d::moya::material::ggxAlbedos(), kernel::spectrum::SampledWavelengths::visible(0.5F),
        glm::normalize(outgoing));
}

/** A normal map's value for a normal tilted towards the tangent by an angle in degrees. **/
scene::Value tiltedBy(float degrees) {
    const float angle = glm::radians(degrees);
    return scene::Value::floats({ 0.5F + (0.5F * std::sin(angle)), 0.5F, 0.5F + (0.5F * std::cos(angle)) });
}

/**
 * A surface with a coat of a weight, whose geometry_normal a normal map of a value gives. The map
 * gives the coat's normal too where asked.
 **/
scene::Handle mapped(scene::Scene & scene, const scene::Value & value, float coat, bool coated) {
    const scene::Handle surface = node(scene, "surface", kOpenPbr, { { "coat_weight", scene::Value::number(coat) } });
    const scene::Handle map = node(scene, "map", "ND_normalmap_float", { { "in", value } });
    connect(scene, map, surface, "geometry_normal");
    if (coated) {
        connect(scene, map, surface, "geometry_coat_normal");
    }
    return surface;
}

/** Whether two directions are the same, within a tolerance. **/
bool same(const glm::vec3 & left, const glm::vec3 & right) {
    return glm::distance(left, right) < 1.0e-4F;
}

/** The direction a normal mirrors another to. **/
glm::vec3 mirrored(const glm::vec3 & direction, const glm::vec3 & normal) {
    return (2.0F * glm::dot(direction, normal) * normal) - direction;
}

// a normal map of the flat value gives the BSDF a surface with no normal map gives: the same
// frames, and the same value in every direction
BOOST_AUTO_TEST_CASE(moya_normal_a_flat_normal_map_changes_nothing_test) {
    scene::Scene flat(scene::kinds());
    place(flat, mapped(flat, scene::Value::floats({ 0.5F, 0.5F, 1.0F }), 0.5F, true));
    scene::Scene plain(scene::kinds());
    place(plain, node(plain, "surface", kOpenPbr, { { "coat_weight", scene::Value::number(0.5F) } }));
    const glm::vec3 outgoing { -0.3F, 0.4F, 1.0F };
    const bsdf::Bsdf mappedBsdf = scatteringAt(flat, outgoing);
    const bsdf::Bsdf plainBsdf = scatteringAt(plain, outgoing);
    BOOST_TEST(same(mappedBsdf.frame.normal, plainBsdf.frame.normal));
    BOOST_TEST(same(mappedBsdf.frame.tangent, plainBsdf.frame.tangent));
    BOOST_TEST(same(mappedBsdf.coatFrame.normal, plainBsdf.coatFrame.normal));
    const glm::vec3 leaving = glm::normalize(outgoing);
    for (const glm::vec3 & incoming : { glm::vec3 { 0.0F, 0.0F, 1.0F }, glm::normalize(glm::vec3 { 0.5F, -0.2F, 0.7F }), glm::normalize(glm::vec3 { -0.1F, 0.6F, 0.3F }) }) {
        BOOST_TEST(mappedBsdf.projected(leaving, incoming)[0] == plainBsdf.projected(leaving, incoming)[0], boost::test_tools::tolerance(1.0e-5F));
    }
}

// a normal map tilted towards the tangent turns the shading normal by its angle, and lays the
// tangent flat against it
BOOST_AUTO_TEST_CASE(moya_normal_a_tilted_normal_map_turns_the_normal_by_its_angle_test) {
    scene::Scene tilted(scene::kinds());
    place(tilted, mapped(tilted, tiltedBy(30.0F), 0.0F, false));
    const bsdf::Bsdf scattering = scatteringAt(tilted, glm::vec3 { 0.0F, 0.0F, 1.0F });
    const float half = std::sqrt(3.0F) / 2.0F;
    BOOST_TEST(same(scattering.frame.normal, glm::vec3 { 0.5F, 0.0F, half }));
    BOOST_TEST(same(scattering.frame.tangent, glm::vec3 { half, 0.0F, -0.5F }));
    BOOST_TEST(same(scattering.frame.bitangent, glm::vec3 { 0.0F, 1.0F, 0.0F }));
}

// a normal so tilted that it would mirror outgoing below the surface is bent back towards the
// geometric normal, in the plane they share, until the mirror just keeps its least cosine
BOOST_AUTO_TEST_CASE(moya_normal_a_normal_facing_away_is_bent_back_test) {
    scene::Scene tilted(scene::kinds());
    place(tilted, mapped(tilted, tiltedBy(80.0F), 0.0F, false));
    const glm::vec3 outgoing = glm::normalize(glm::vec3 { -0.6F, 0.0F, 0.8F });
    const glm::vec3 given { std::sin(glm::radians(80.0F)), 0.0F, std::cos(glm::radians(80.0F)) };
    BOOST_TEST_REQUIRE(mirrored(outgoing, given).z < 0.0F);
    const glm::vec3 bent = scatteringAt(tilted, outgoing).frame.normal;
    BOOST_TEST(mirrored(outgoing, bent).z == kernel::surface::kLeastRise, boost::test_tools::tolerance(1.0e-3F));
    BOOST_TEST(bent.y == 0.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(bent.x > 0.0F);
    BOOST_TEST(bent.z > given.z);
    // a normal whose mirror already rises is kept, from either side of the surface
    const glm::vec3 kept = glm::normalize(glm::vec3 { 0.2F, 0.1F, 1.0F });
    BOOST_TEST(same(kernel::surface::visible(kept, { 0.0F, 0.0F, 1.0F }, outgoing), kept));
    BOOST_TEST(same(kernel::surface::visible(kept, { 0.0F, 0.0F, 1.0F }, -outgoing), kept));
}

// the coat keeps the surface's own normal while only the base is mapped, and takes the map once
// its own normal is connected
BOOST_AUTO_TEST_CASE(moya_normal_the_coat_keeps_its_own_normal_test) {
    const glm::vec3 up { 0.0F, 0.0F, 1.0F };
    const glm::vec3 turned { 0.5F, 0.0F, std::sqrt(3.0F) / 2.0F };
    scene::Scene base(scene::kinds());
    place(base, mapped(base, tiltedBy(30.0F), 1.0F, false));
    const bsdf::Bsdf underCoat = scatteringAt(base, up);
    BOOST_TEST(same(underCoat.frame.normal, turned));
    BOOST_TEST(same(underCoat.coatFrame.normal, up));
    scene::Scene both(scene::kinds());
    place(both, mapped(both, tiltedBy(30.0F), 1.0F, true));
    BOOST_TEST(same(scatteringAt(both, up).coatFrame.normal, turned));
}

/**
 * A surface whose geometry_normal a bump of a height gives: s, the square's x, or a constant
 * where one is given.
 **/
scene::Handle bumped(scene::Scene & scene, float scale, std::optional<float> constant) {
    const scene::Handle surface = node(scene, "surface", kOpenPbr, {});
    std::vector<Given> inputs { { "scale", scene::Value::number(scale) } };
    if (constant) {
        inputs.push_back({ "height", scene::Value::number(*constant) });
    }
    const scene::Handle bump = node(scene, "bump", "ND_bump_vector3", inputs);
    if (!constant) {
        const scene::Handle uv = node(scene, "uv", "ND_texcoord_vector2", {});
        const scene::Handle s = node(scene, "s", "ND_extract_vector2", { { "index", scene::Value::integer(0) } });
        connect(scene, uv, s, "in");
        connect(scene, s, bump, "height");
    }
    connect(scene, bump, surface, "geometry_normal");
    return surface;
}

// a bump of a constant height leaves the normal as it was
BOOST_AUTO_TEST_CASE(moya_normal_a_bump_of_a_constant_height_changes_nothing_test) {
    scene::Scene constant(scene::kinds());
    place(constant, bumped(constant, 16.0F, 0.7F));
    BOOST_TEST(same(scatteringAt(constant, glm::vec3 { 0.0F, 0.0F, 1.0F }).frame.normal, glm::vec3 { 0.0F, 0.0F, 1.0F }));
    BOOST_TEST(same(scatteringAt(constant, glm::vec3 { 0.0F, 0.0F, 1.0F }, 0.05F).frame.normal, glm::vec3 { 0.0F, 0.0F, 1.0F }));
}

// a height that rises with s tilts the normal away from the tangent by its slope. heighttonormal
// takes a sixteenth of it, and the bump scales that, so a scale of sixteen tilts it by 45 degrees.
// The differences are taken over the footprint where it has an area, and over a small step in s
// and t where it has none, and both give the same slope
BOOST_AUTO_TEST_CASE(moya_normal_a_height_that_rises_tilts_the_normal_by_its_slope_test) {
    scene::Scene rising(scene::kinds());
    place(rising, bumped(rising, 16.0F, std::nullopt));
    const glm::vec3 expected = glm::normalize(glm::vec3 { -1.0F, 0.0F, 1.0F });
    // seen from the side it tilts towards, so its mirror of outgoing stays above the surface
    const glm::vec3 outgoing { -0.5F, 0.0F, 1.0F };
    BOOST_TEST(same(scatteringAt(rising, outgoing).frame.normal, expected));
    BOOST_TEST(same(scatteringAt(rising, outgoing, 0.05F).frame.normal, expected));
    scene::Scene gentle(scene::kinds());
    place(gentle, bumped(gentle, 1.0F, std::nullopt));
    BOOST_TEST(same(scatteringAt(gentle, glm::vec3 { 0.0F, 0.0F, 1.0F }).frame.normal, glm::normalize(glm::vec3 { -1.0F / 16.0F, 0.0F, 1.0F })));
}

};  // namespace
