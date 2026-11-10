/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Placed.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/surface/Frame.h>
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
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** A scene's surfaces and its hits for some rays, through the hierarchy. **/
struct Shaded {
    geometry::Prototypes prototypes;
    std::vector<kernel::geometry::Ray> rays;
    std::vector<kernel::surface::ShadingPoint> points;
};

/** A primvar's declaration of three floats a value, of any count. **/
scene::Declaration primvar(const std::string & name, scene::Type type, scene::Interpolation interpolation) {
    return scene::Declaration { .name = name,
        .type = type,
        .count = scene::Declaration::kAny,
        .fallback = type == scene::Type::Colour ? scene::Value::colours({}) : scene::Value::normals({}),
        .interpolation = interpolation };
}

/** A ray from a point towards another, at a time. **/
kernel::geometry::Ray towards(const glm::vec3 & from, const glm::vec3 & to, float time = 0.0F) {
    kernel::geometry::Ray ray;
    ray.origin = from;
    ray.direction = glm::normalize(to - from);
    ray.time = time;
    return ray;
}

/** The shading point of each ray's nearest hit through a scene's hierarchy. **/
Shaded shade(const scene::Scene & scene, const std::vector<kernel::geometry::Ray> & rays) {
    Shaded out { .prototypes = geometry::gather(scene, {}), .rays = rays, .points = {} };
    CpuDevice device;
    const view::Traced traced = view::trace(device, out.prototypes, rays, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, out.prototypes, colour::Config());
    const kernel::surface::Surfaces view = surfaces.view();
    for (std::size_t i = 0; i < rays.size(); ++i) {
        out.points.push_back(view.at(traced.nearest[i], rays[i]));
    }
    return out;
}

/** A primvar's value at a shading point, read through a scene's surfaces. **/
bool readAt(const scene::Scene & scene, const kernel::geometry::Ray & ray, std::uint32_t name, glm::vec3 & out) {
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    CpuDevice device;
    const view::Traced traced = view::trace(device, prototypes, { ray }, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    float value[3] {};
    const bool found = held.read(held.at(traced.nearest.front(), ray), name, value, 3U);
    out = glm::vec3 { value[0], value[1], value[2] };
    return found;
}

/** A square of side two about the origin in z = 0, facing +z, as one face of four corners. **/
scene::Handle square(scene::Scene & scene, const std::string & name) {
    const scene::Handle out = scene.create("mesh", name);
    scene.set(out, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(out, "vertices", scene::Value::integers({ 4 }));
    scene.set(out, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    return out;
}

/** An octahedron of radius one about the origin, with or without N out from its centre. **/
scene::Handle octahedron(scene::Scene & scene, bool normals) {
    const std::vector<glm::vec3> vertices { { 1.0F, 0.0F, 0.0F }, { -1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, -1.0F, 0.0F },
        { 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F, -1.0F } };
    const scene::Handle out = scene.create("mesh", "octahedron");
    scene.set(out, "P", scene::Value::points(vertices));
    scene.set(out, "vertices", scene::Value::integers(std::vector<std::int32_t>(8, 3)));
    scene.set(out, "indices", scene::Value::integers({ 0, 2, 4, 2, 1, 4, 1, 3, 4, 3, 0, 4, 2, 0, 5, 1, 2, 5, 3, 1, 5, 0, 3, 5 }));
    if (normals) {
        scene.declare(out, primvar("N", scene::Type::Normal, scene::Interpolation::Vertex));
        scene.set(out, "N", scene::Value::normals(vertices));
    }
    return out;
}

/** Rays from far around the origin towards points near it, at a time. **/
std::vector<kernel::geometry::Ray> inwards(std::size_t count, float reach, std::uint32_t & seed, float time = 0.0F) {
    std::vector<kernel::geometry::Ray> out;
    for (std::size_t i = 0; i < count; ++i) {
        const glm::vec3 from = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * 10.0F;
        const glm::vec3 to = (glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * reach;
        out.push_back(towards(from, to, time));
    }
    return out;
}

};  // namespace

// a shading point lies where the ray met the surface, with the normal and face the placed scene
// gives the same hit. The scene holds meshes that bend, quadrics and points, through moving and
// nested instances, and is met at several times
BOOST_AUTO_TEST_CASE(moya_surfaces_a_shading_point_agrees_with_the_placed_scene_test) {
    scene::Scene scene(scene::kinds());
    shutter(scene);
    std::uint32_t seed = 7;
    const std::vector<glm::vec3> start = scattered(60, 4.0F, 0.6F, seed);
    std::vector<glm::vec3> finish = start;
    for (glm::vec3 & point : finish) {
        point += glm::vec3 { 0.3F, 0.0F, 0.2F };
    }
    const scene::Handle bending = mesh(scene, "bending", { 0.0F, 1.0F }, { start, finish });
    const scene::Handle ball = scene.create("sphere", "ball");
    scene.set(ball, "radius", scene::Value::number(0.8F));
    const scene::Handle ring = scene.create("torus", "ring");
    scene.set(ring, "majorRadius", scene::Value::number(1.0F));
    scene.set(ring, "minorRadius", scene::Value::number(0.3F));
    const scene::Handle cloud = scene.create("points", "cloud");
    scene.set(cloud, "P", scene::Value::points(scattered(10, 3.0F, 0.0F, seed)));
    scene.set(cloud, "constantWidth", scene::Value::floats({ 0.5F }));
    const scene::Handle group = scene.create("group", "group");
    instanced(scene, "ball in group", ball, group, moved(1.0F, 0.0F, 0.0F));
    instanced(scene, "ring in group", ring, group, {}, { glm::rotate(glm::mat4(1.0F), 0.4F, glm::vec3 { 1.0F, 0.0F, 0.0F }) });
    instanced(scene, "bending", bending, {}, moved(-2.0F, -2.0F, -2.0F));
    instanced(scene, "cloud", cloud, {}, moved(-1.5F, -1.5F, 1.0F));
    instanced(scene, "group", group, {}, { 0.0F, 1.0F }, { moved(0.0F, 1.0F, 0.0F), glm::rotate(moved(0.5F, 1.5F, 0.0F), 0.8F, glm::vec3 { 0.0F, 0.0F, 1.0F }) });
    instanced(scene, "group again", group, {}, glm::scale(moved(0.0F, -2.5F, 0.0F), glm::vec3 { 1.0F, 2.0F, 0.5F }));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.empty());
    CpuDevice device;
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    std::size_t met = 0;
    for (const float time : { 0.0F, 0.3F, 0.75F, 1.0F }) {
        const std::vector<kernel::geometry::Ray> rays = inwards(1500, 6.0F, seed, time);
        const view::Traced traced = view::trace(device, prototypes, rays, view::Tracer::Hierarchy);
        const geometry::World world(device, geometry::flatten(prototypes, time));
        const kernel::geometry::Placed placed = world.view();
        for (std::size_t i = 0; i < rays.size(); ++i) {
            const kernel::geometry::Hit & hit = traced.nearest[i];
            const kernel::surface::ShadingPoint point = held.at(hit, rays[i]);
            BOOST_TEST_CONTEXT("ray " << i << " at " << time) {
                BOOST_TEST(point.found() == hit.found());
                if (!hit.found()) {
                    continue;
                }
                ++met;
                const glm::vec3 along = rays[i].at(hit.distance);
                BOOST_TEST(glm::distance(point.position, along) <= 1.0e-3F * std::max(1.0F, hit.distance));
                BOOST_TEST(glm::dot(point.geometric, placed.normal(hit, rays[i])) >= 0.999F);
                BOOST_TEST(point.face == placed.face(hit));
                BOOST_TEST(point.front == (glm::dot(rays[i].direction, point.geometric) < 0.0F));
                BOOST_TEST(glm::dot(point.shading, point.geometric) >= 0.0F);
            }
        }
    }
    BOOST_TEST(met > 1000U);
}

// a sphere's shading normal at every hit is the direction from its centre, and the frame around
// it is of three directions at right angles
BOOST_AUTO_TEST_CASE(moya_surfaces_a_spheres_shading_normal_is_its_radius_direction_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle ball = scene.create("sphere", "ball");
    scene.set(ball, "radius", scene::Value::number(2.0F));
    const glm::vec3 centre { 3.0F, -1.0F, 2.0F };
    instanced(scene, "ball", ball, {}, moved(centre.x, centre.y, centre.z));
    scene.commit();
    scene.apply();
    std::uint32_t seed = 3;
    std::vector<kernel::geometry::Ray> rays = inwards(200, 2.0F, seed);
    for (kernel::geometry::Ray & ray : rays) {
        ray.origin += centre;
    }
    const Shaded shaded = shade(scene, rays);
    std::size_t met = 0;
    for (const kernel::surface::ShadingPoint & point : shaded.points) {
        if (!point.found()) {
            continue;
        }
        ++met;
        const glm::vec3 outward = point.position - centre;
        BOOST_TEST(glm::length(outward) == 2.0F, boost::test_tools::tolerance(1.0e-4F));
        BOOST_TEST(glm::dot(point.shading, glm::normalize(outward)) >= 0.9999F);
        BOOST_TEST(std::fabs(glm::dot(point.frame.tangent, point.frame.bitangent)) <= 1.0e-5F);
        BOOST_TEST(std::fabs(glm::dot(point.frame.tangent, point.frame.normal)) <= 1.0e-5F);
        BOOST_TEST(glm::length(point.frame.tangent) == 1.0F, boost::test_tools::tolerance(1.0e-5F));
        BOOST_TEST(glm::dot(glm::cross(point.frame.tangent, point.frame.bitangent), point.frame.normal) >= 0.9999F);
        BOOST_TEST(glm::distance(point.leave, point.position) == 0.0F);
    }
    BOOST_TEST(met > 100U);
}

// a mesh with N at each vertex shades with the normal interpolated across the triangle it met, and
// one without shades with its triangle's own normal
BOOST_AUTO_TEST_CASE(moya_surfaces_a_mesh_shades_with_its_interpolated_normal_test) {
    const glm::vec3 normals[3] { glm::normalize(glm::vec3 { -0.5F, 0.0F, 1.0F }), glm::normalize(glm::vec3 { 0.5F, 0.2F, 1.0F }),
        glm::normalize(glm::vec3 { 0.0F, 0.6F, 1.0F }) };
    const std::vector<glm::vec3> corners { { 0.0F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F }, { 0.0F, 2.0F, 0.0F } };
    const glm::vec3 at { 0.4F, 0.6F, 0.0F };
    const kernel::geometry::Ray ray = towards(at + glm::vec3 { 0.0F, 0.0F, 5.0F }, at);

    for (const bool smooth : { true, false }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle triangle = mesh(scene, "triangle", corners);
        if (smooth) {
            scene.declare(triangle, primvar("N", scene::Type::Normal, scene::Interpolation::Vertex));
            scene.set(triangle, "N", scene::Value::normals({ normals[0], normals[1], normals[2] }));
        }
        instanced(scene, "triangle", triangle, {}, glm::mat4(1.0F));
        scene.commit();
        scene.apply();
        const kernel::surface::ShadingPoint point = shade(scene, { ray }).points.front();
        BOOST_TEST_CONTEXT((smooth ? "with N" : "without N")) {
            BOOST_TEST_REQUIRE(point.found());
            BOOST_TEST(glm::dot(point.geometric, glm::vec3 { 0.0F, 0.0F, 1.0F }) >= 0.9999F);
            const glm::vec3 weights { 1.0F - 0.2F - 0.3F, 0.2F, 0.3F };
            const glm::vec3 expected = smooth ? glm::normalize((normals[0] * weights.x) + (normals[1] * weights.y) + (normals[2] * weights.z)) :
                                                glm::vec3 { 0.0F, 0.0F, 1.0F };
            BOOST_TEST(glm::dot(point.shading, expected) >= 0.9999F);
            BOOST_TEST(point.weights.y == 0.2F, boost::test_tools::tolerance(1.0e-4F));
            BOOST_TEST(point.weights.z == 0.3F, boost::test_tools::tolerance(1.0e-4F));
        }
    }
}

// a face-varying colour is read at each corner of a face, and a uniform one is its face's
BOOST_AUTO_TEST_CASE(moya_surfaces_a_colour_is_read_by_its_interpolation_test) {
    const glm::vec3 colours[4] { { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F, 1.0F }, { 1.0F, 1.0F, 0.0F } };
    const glm::vec3 near[4] { { -0.99F, -0.99F, 0.0F }, { 0.99F, -0.99F, 0.0F }, { 0.99F, 0.99F, 0.0F }, { -0.99F, 0.99F, 0.0F } };
    scene::Scene scene(scene::kinds());
    const scene::Handle face = square(scene, "square");
    scene.declare(face, primvar("Cs", scene::Type::Colour, scene::Interpolation::FaceVarying));
    scene.set(face, "Cs", scene::Value::colours({ colours[0], colours[1], colours[2], colours[3] }));
    instanced(scene, "square", face, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    for (int corner = 0; corner < 4; ++corner) {
        glm::vec3 read { 0.0F };
        BOOST_TEST_CONTEXT("corner " << corner) {
            BOOST_TEST_REQUIRE(readAt(scene, towards(near[corner] + glm::vec3 { 0.0F, 0.0F, 3.0F }, near[corner]), kernel::surface::kColour, read));
            BOOST_TEST(glm::distance(read, colours[corner]) <= 0.02F);
        }
    }

    scene::Scene faces(scene::kinds());
    const scene::Handle two = faces.create("mesh", "two");
    faces.set(two, "P", scene::Value::points({ { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 2.0F, 0.0F, 0.0F },
        { 2.0F, 1.0F, 0.0F } }));
    faces.set(two, "vertices", scene::Value::integers({ 4, 4 }));
    faces.set(two, "indices", scene::Value::integers({ 0, 1, 2, 3, 1, 4, 5, 2 }));
    faces.declare(two, primvar("Cs", scene::Type::Colour, scene::Interpolation::Uniform));
    faces.set(two, "Cs", scene::Value::colours({ colours[0], colours[2] }));
    instanced(faces, "two", two, {}, glm::mat4(1.0F));
    faces.commit();
    faces.apply();
    glm::vec3 read { 0.0F };
    BOOST_TEST_REQUIRE(readAt(faces, towards({ 1.5F, 0.4F, 3.0F }, { 1.5F, 0.4F, 0.0F }), kernel::surface::kColour, read));
    BOOST_TEST(glm::distance(read, colours[2]) <= 1.0e-6F);
}

// a colour named in another space is read in ACEScg, and one that names none in the working space
BOOST_AUTO_TEST_CASE(moya_surfaces_a_colour_is_converted_into_acescg_test) {
    const glm::vec3 given { 0.8F, 0.3F, 0.1F };
    const colour::Config config;
    for (const std::string & space : { std::string(), std::string("Linear Rec.709 (sRGB)") }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle face = square(scene, "square");
        scene.declare(face, primvar("Cs", scene::Type::Colour, scene::Interpolation::Constant));
        scene.set(face, "Cs", scene::Value::colours({ given }, space));
        instanced(scene, "square", face, {}, glm::mat4(1.0F));
        scene.commit();
        scene.apply();
        glm::vec3 read { 0.0F };
        BOOST_TEST_CONTEXT("space " << space) {
            BOOST_TEST_REQUIRE(readAt(scene, towards({ 0.1F, 0.2F, 3.0F }, { 0.1F, 0.2F, 0.0F }), kernel::surface::kColour, read));
            BOOST_TEST(glm::distance(read, config.input(space).apply(given)) <= 1.0e-5F);
        }
    }
}

// a ray leaving a coarse sphere of smooth normals starts out on the curved surface its corners
// and their normals describe, which is Phong's. It is no nearer the centre than the triangle it
// met. An octahedron's Phong surface bulges furthest at the middle of each edge, to 0.75 along
// both of its axes, a length of 1.0607. A sphere without normals leaves from where it was met
BOOST_AUTO_TEST_CASE(moya_surfaces_a_coarse_sphere_leaves_from_its_curved_surface_test) {
    for (const bool normals : { true, false }) {
        scene::Scene scene(scene::kinds());
        instanced(scene, "octahedron", octahedron(scene, normals), {}, glm::mat4(1.0F));
        scene.commit();
        scene.apply();
        std::uint32_t seed = 11;
        const Shaded shaded = shade(scene, inwards(400, 0.5F, seed));
        std::size_t moved = 0;
        std::size_t met = 0;
        for (const kernel::surface::ShadingPoint & point : shaded.points) {
            if (!point.found()) {
                continue;
            }
            ++met;
            BOOST_TEST_CONTEXT((normals ? "with N" : "without N")) {
                BOOST_TEST(glm::length(point.leave) >= glm::length(point.position) - 1.0e-6F);
                BOOST_TEST(glm::length(point.leave) <= 1.0607F);
                moved += glm::length(point.leave) > glm::length(point.position) + 0.01F ? 1 : 0;
                if (!normals) {
                    BOOST_TEST(glm::distance(point.leave, point.position) == 0.0F);
                }
            }
        }
        BOOST_TEST(met > 300U);
        BOOST_TEST((normals ? moved > met / 2 : moved == 0U));
    }
}

// an undisplaced subdivision surface shades with its limit normals, which turn smoothly across the
// triangles it is cut into while their own normals turn in steps
BOOST_AUTO_TEST_CASE(moya_surfaces_a_subdivision_surface_shades_with_its_limit_normal_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle cube = scene.create("subdivision", "cube");
    scene.set(cube, "P", scene::Value::points({ { -1.0F, -1.0F, -1.0F }, { 1.0F, -1.0F, -1.0F }, { 1.0F, 1.0F, -1.0F }, { -1.0F, 1.0F, -1.0F },
        { -1.0F, -1.0F, 1.0F }, { 1.0F, -1.0F, 1.0F }, { 1.0F, 1.0F, 1.0F }, { -1.0F, 1.0F, 1.0F } }));
    scene.set(cube, "vertices", scene::Value::integers(std::vector<std::int32_t>(6, 4)));
    scene.set(cube, "indices",
        scene::Value::integers({ 0, 3, 2, 1, 4, 5, 6, 7, 0, 1, 5, 4, 2, 3, 7, 6, 1, 2, 6, 5, 3, 0, 4, 7 }));
    instanced(scene, "cube", cube, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    // a row of rays across the surface, each a little along from the one before
    std::vector<kernel::geometry::Ray> rays;
    for (int i = 0; i < 400; ++i) {
        const float x = -0.6F + (1.2F * static_cast<float>(i) / 400.0F);
        rays.push_back(towards({ x, 0.13F, 5.0F }, { x, 0.13F, 0.0F }));
    }
    const Shaded shaded = shade(scene, rays);
    float shadingStep = 0.0F;
    float geometricStep = 0.0F;
    for (std::size_t i = 1; i < shaded.points.size(); ++i) {
        BOOST_TEST_REQUIRE(shaded.points[i].found());
        shadingStep = std::max(shadingStep, 1.0F - glm::dot(shaded.points[i].shading, shaded.points[i - 1].shading));
        geometricStep = std::max(geometricStep, 1.0F - glm::dot(shaded.points[i].geometric, shaded.points[i - 1].geometric));
    }
    BOOST_TEST(geometricStep > 1.0e-4F);
    BOOST_TEST(shadingStep < geometricStep / 10.0F);
}

// a primvar a geometry cannot hold is reported and left out, and the rest are held
BOOST_AUTO_TEST_CASE(moya_surfaces_a_primvar_that_cannot_be_held_is_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle ball = scene.create("sphere", "ball");
    scene.declare(ball, primvar("Cs", scene::Type::Colour, scene::Interpolation::Varying));
    scene.set(ball, "Cs", scene::Value::colours({ glm::vec3 { 1.0F }, glm::vec3 { 1.0F }, glm::vec3 { 1.0F }, glm::vec3 { 1.0F } }));
    const scene::Handle face = square(scene, "square");
    scene.declare(face, primvar("Cs", scene::Type::Colour, scene::Interpolation::Vertex));
    scene.set(face, "Cs", scene::Value::colours({ glm::vec3 { 1.0F } }));
    const scene::Handle other = square(scene, "other");
    scene.declare(other, primvar("N", scene::Type::Normal, scene::Interpolation::Constant));
    scene.set(other, "N", scene::Value::normals({ glm::vec3 { 0.0F, 0.0F, 1.0F } }));
    instanced(scene, "ball", ball, {}, glm::mat4(1.0F));
    instanced(scene, "square", face, {}, glm::mat4(1.0F));
    instanced(scene, "other", other, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "primvar Cs of ball is left out, because a varying primvar on a quadric"));
    BOOST_TEST(reported(prototypes, "primvar Cs of square is left out, because a vertex primvar of 1 values where it needs 4"));
    BOOST_TEST(prototypes.primvars.size() == 1U);
}

// a geometry placed again from the cache keeps its primvars, where another made after it moves
// them along
BOOST_AUTO_TEST_CASE(moya_surfaces_a_cached_geometry_keeps_its_primvars_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle first = square(scene, "first");
    const scene::Handle second = square(scene, "second");
    for (const auto & [handle, value] : { std::pair { first, glm::vec3 { 0.2F, 0.4F, 0.6F } }, std::pair { second, glm::vec3 { 0.9F, 0.1F, 0.5F } } }) {
        scene.declare(handle, primvar("Cs", scene::Type::Colour, scene::Interpolation::Constant));
        scene.set(handle, "Cs", scene::Value::colours({ value }));
    }
    instanced(scene, "first", first, {}, glm::mat4(1.0F));
    instanced(scene, "second", second, {}, moved(5.0F, 0.0F, 0.0F));
    scene.commit();
    scene.apply();
    geometry::Cache cache;
    const geometry::Prototypes made = geometry::gather(scene, {}, cache);
    cache.forget(first.index);
    const geometry::Prototypes again = geometry::gather(scene, {}, cache);
    BOOST_TEST(cache.made().size() == 1U);
    for (const geometry::Prototypes * prototypes : { &made, &again }) {
        for (const geometry::Geometry & held : prototypes->geometries) {
            BOOST_TEST_REQUIRE(held.primvarCount == 1U);
            const kernel::surface::Primvar & colour = prototypes->primvars[held.primvars];
            const glm::vec3 value { prototypes->primvarValues[colour.first], prototypes->primvarValues[colour.first + 1],
                prototypes->primvarValues[colour.first + 2] };
            BOOST_TEST(glm::distance(value, held.id == first.index ? glm::vec3 { 0.2F, 0.4F, 0.6F } : glm::vec3 { 0.9F, 0.1F, 0.5F }) == 0.0F);
        }
    }
}

// a hit that met nothing, or names an instance the surfaces do not hold, is no shading point
BOOST_AUTO_TEST_CASE(moya_surfaces_a_hit_of_nothing_is_no_shading_point_test) {
    scene::Scene scene(scene::kinds());
    instanced(scene, "square", square(scene, "square"), {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    CpuDevice device;
    const geometry::Surfaces surfaces(device, prototypes, colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    const kernel::geometry::Ray ray = towards({ 0.0F, 0.0F, 3.0F }, glm::vec3 { 0.0F });
    BOOST_TEST(!held.at(kernel::geometry::Hit {}, ray).found());
    kernel::geometry::Hit stranger;
    stranger.distance = 3.0F;
    stranger.primitive = 0;
    stranger.depth = 1;
    stranger.instances[0] = 999;
    BOOST_TEST(!held.at(stranger, ray).found());
}

};  // namespace v3d::moya::tracing
