/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Flat.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surface.h>
#include <moya/libmoya/geometry/Tessellation.h>
#include <moya/libmoya/geometry/Topology.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Hits.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace cpu = v3d::moya::device::cpu;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;
typedef scene::Interpolation Interpolation;

/** The corners of a cube from -1 to 1. Corner i takes x from bit 0, y from bit 1, z from bit 2. **/
std::vector<glm::vec3> cubePoints() {
    std::vector<glm::vec3> out;
    out.reserve(8);
    for (int corner = 0; corner < 8; ++corner) {
        out.emplace_back((corner & 1) != 0 ? 1.0F : -1.0F, (corner & 2) != 0 ? 1.0F : -1.0F, (corner & 4) != 0 ? 1.0F : -1.0F);
    }
    return out;
}

/** The cube's six faces, wound outward. The fifth is its top, at z = 1, from its corner 4. **/
geometry::Topology cube() {
    geometry::Topology out;
    out.vertices = std::vector<std::int32_t>(6, 4);
    out.indices = { 0, 2, 3, 1, 0, 1, 5, 4, 0, 4, 6, 2, 1, 3, 7, 5, 4, 5, 7, 6, 2, 6, 7, 3 };
    out.points = 8;
    return out;
}

/** The floats of a list of points, three to a point. **/
std::vector<float> flat(const std::vector<glm::vec3> & points) {
    std::vector<float> out;
    for (const glm::vec3 & point : points) {
        out.insert(out.end(), { point.x, point.y, point.z });
    }
    return out;
}

/** The points of a primvar three floats wide. **/
std::vector<glm::vec3> pointsOf(const std::vector<float> & floats) {
    std::vector<glm::vec3> out;
    for (std::size_t at = 0; at + 2 < floats.size(); at += 3) {
        out.emplace_back(floats[at], floats[at + 1], floats[at + 2]);
    }
    return out;
}

/** A tessellation of chosen places, each on a patch face of a face, and no triangles. **/
geometry::Tessellation placesAt(const std::vector<std::uint32_t> & patches, const std::vector<glm::vec2> & positions) {
    geometry::Tessellation out;
    out.patches = patches;
    out.positions = positions;
    out.placeFaces = patches;
    return out;
}

/** A subdivision surface of the scene through points, with a topology's faces and tags. **/
scene::Handle subdivision(scene::Scene & scene, const std::string & name, const geometry::Topology & topology, const std::vector<glm::vec3> & points) {
    const scene::Handle out = scene.create("subdivision", name);
    scene.set(out, "P", scene::Value::points(points));
    scene.set(out, "vertices", scene::Value::integers(topology.vertices));
    scene.set(out, "indices", scene::Value::integers(topology.indices));
    return out;
}

// a cube's corner and the middle of its faces lie on Catmull-Clark's limit surface, at a half
// along each axis and 68/81 from its centre, and every place of the surface lies between them
BOOST_AUTO_TEST_CASE(moya_subdivision_a_cube_reaches_catmull_clarks_limit_test) {
    const geometry::Surface surface(cube());
    const std::vector<float> points = flat(cubePoints());
    const std::vector<glm::vec3> limit =
        pointsOf(surface.at(points, 3, Interpolation::Vertex, placesAt({ 4, 4 }, { glm::vec2(0.0F), glm::vec2(0.5F) })));
    BOOST_TEST(glm::length(limit[0] - glm::vec3 { -0.5F, -0.5F, 0.5F }) < 1.0e-5F);
    BOOST_TEST(glm::length(limit[1] - glm::vec3 { 0.0F, 0.0F, 68.0F / 81.0F }) < 1.0e-5F);

    const geometry::Tessellation cut = surface.cut(8);
    BOOST_TEST(cut.faces.size() == 6U * 8U * 8U * 2U);
    for (const glm::vec3 & place : pointsOf(surface.at(points, 3, Interpolation::Vertex, cut))) {
        BOOST_TEST((glm::length(place) > (68.0F / 81.0F) - 1.0e-5F && glm::length(place) < std::sqrt(0.75F) + 1.0e-5F));
    }
}

// a crease of infinite sharpness around the cube's top keeps that loop at the top's height, and
// its corners where the crease's own B-spline puts them
BOOST_AUTO_TEST_CASE(moya_subdivision_a_crease_of_infinite_sharpness_keeps_its_edge_test) {
    geometry::Topology creased = cube();
    creased.creases = { 4, 5, 7, 6, 4 };
    creased.creaseLengths = { 5 };
    creased.creaseSharpness = { 10.0F };
    const std::vector<float> points = flat(cubePoints());
    // the top face's edges, from its corner 4 along each side
    const geometry::Tessellation edges = placesAt({ 4, 4, 4, 4, 4 },
        { glm::vec2(0.0F), glm::vec2(0.3F, 0.0F), glm::vec2(1.0F, 0.6F), glm::vec2(0.2F, 1.0F), glm::vec2(0.0F, 0.9F) });
    const std::vector<glm::vec3> sharp = pointsOf(geometry::Surface(creased).at(points, 3, Interpolation::Vertex, edges));
    for (const glm::vec3 & place : sharp) {
        BOOST_TEST(std::fabs(place.z - 1.0F) < 1.0e-5F, place.z);
    }
    BOOST_TEST(glm::length(sharp[0] - glm::vec3 { -2.0F / 3.0F, -2.0F / 3.0F, 1.0F }) < 1.0e-5F);
    // without the crease the same places sink below the top
    for (const glm::vec3 & place : pointsOf(geometry::Surface(cube()).at(points, 3, Interpolation::Vertex, edges))) {
        BOOST_TEST(place.z < 0.95F);
    }
}

// a hole leaves its face open and every other face cut, and a hole that names no face is refused
BOOST_AUTO_TEST_CASE(moya_subdivision_a_hole_leaves_its_face_open_test) {
    geometry::Topology holed = cube();
    holed.holes = { 4 };
    const geometry::Tessellation cut = geometry::Surface(holed).cut(4);
    const std::set<std::uint32_t> faces(cut.faces.begin(), cut.faces.end());
    BOOST_TEST((faces == std::set<std::uint32_t> { 0, 1, 2, 3, 5 }));
    BOOST_TEST(cut.faces.size() == 5U * 4U * 4U * 2U);
    holed.holes = { 6 };
    BOOST_CHECK_THROW(geometry::Surface { holed }, std::invalid_argument);
}

// a boundary that is not interpolated leaves the faces along it open, and one that is cuts them
BOOST_AUTO_TEST_CASE(moya_subdivision_a_boundary_is_open_until_it_is_interpolated_test) {
    // a grid of three by three faces, through four by four points
    geometry::Topology grid;
    for (std::int32_t row = 0; row < 3; ++row) {
        for (std::int32_t column = 0; column < 3; ++column) {
            const std::int32_t corner = (row * 4) + column;
            grid.indices.insert(grid.indices.end(), { corner, corner + 1, corner + 5, corner + 4 });
            grid.vertices.push_back(4);
        }
    }
    grid.points = 16;
    const auto faces = [&](geometry::Boundary boundary) {
        grid.boundary = boundary;
        const geometry::Tessellation cut = geometry::Surface(grid).cut(2);
        return std::set<std::uint32_t>(cut.faces.begin(), cut.faces.end()).size();
    };
    BOOST_TEST(faces(geometry::Boundary::None) == 1U);
    BOOST_TEST(faces(geometry::Boundary::Edges) == 9U);
    BOOST_TEST(faces(geometry::Boundary::EdgesAndCorners) == 9U);
}

// an octahedron's vertex reaches Loop's limit for a vertex of four neighbours, and a bilinear
// cube's surface is its control mesh
BOOST_AUTO_TEST_CASE(moya_subdivision_loop_and_bilinear_reach_their_limits_test) {
    geometry::Topology octahedron;
    octahedron.scheme = geometry::Scheme::Loop;
    octahedron.points = 6;
    // the top is 0 and the bottom 1, and the middle runs +x, +y, -x, -y
    octahedron.indices = { 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 2, 1, 3, 2, 1, 4, 3, 1, 5, 4, 1, 2, 5 };
    octahedron.vertices = std::vector<std::int32_t>(8, 3);
    const std::vector<float> points = flat({ { 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F, -1.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F },
        { -1.0F, 0.0F, 0.0F }, { 0.0F, -1.0F, 0.0F } });
    const geometry::Surface loop(octahedron);
    const std::vector<glm::vec3> top = pointsOf(loop.at(points, 3, Interpolation::Vertex, placesAt({ 0 }, { glm::vec2(0.0F) })));
    BOOST_TEST(glm::length(top[0] - glm::vec3 { 0.0F, 0.0F, 768.0F / 1760.0F }) < 1.0e-5F);
    BOOST_TEST(loop.cut(4).faces.size() == 8U * 4U * 4U);

    geometry::Topology box = cube();
    box.scheme = geometry::Scheme::Bilinear;
    const geometry::Surface bilinear(box);
    for (const glm::vec3 & place : pointsOf(bilinear.at(flat(cubePoints()), 3, Interpolation::Vertex, bilinear.cut(3)))) {
        const float reach = std::max({ std::fabs(place.x), std::fabs(place.y), std::fabs(place.z) });
        BOOST_TEST(std::fabs(reach - 1.0F) < 1.0e-5F);
    }
}

// a triangular prism's triangles are cut into a patch face at each corner, and every triangle of
// the closed surface faces outward, as the faces are wound
BOOST_AUTO_TEST_CASE(moya_subdivision_every_patch_face_keeps_its_faces_winding_test) {
    geometry::Topology prism;
    prism.points = 6;
    // a triangle at z = -1 and one at z = 1, and the three sides between
    prism.indices = { 0, 2, 1, 3, 4, 5, 0, 1, 4, 3, 1, 2, 5, 4, 2, 0, 3, 5 };
    prism.vertices = { 3, 3, 4, 4, 4 };
    const std::vector<glm::vec3> corners { { 1.0F, 0.0F, -1.0F }, { -0.5F, 0.866F, -1.0F }, { -0.5F, -0.866F, -1.0F }, { 1.0F, 0.0F, 1.0F },
        { -0.5F, 0.866F, 1.0F }, { -0.5F, -0.866F, 1.0F } };
    const geometry::Surface surface(prism);
    const geometry::Tessellation cut = surface.cut(3);
    BOOST_TEST(cut.faces.size() == (3U + 3U + 3U) * 3U * 3U * 2U);
    const std::vector<glm::vec3> places = pointsOf(surface.at(flat(corners), 3, Interpolation::Vertex, cut));
    std::size_t inward = 0;
    for (std::size_t triangle = 0; triangle < cut.faces.size(); ++triangle) {
        const glm::vec3 & first = places[cut.corners[triangle * 3]];
        const glm::vec3 & second = places[cut.corners[(triangle * 3) + 1]];
        const glm::vec3 & third = places[cut.corners[(triangle * 3) + 2]];
        inward += glm::dot(glm::cross(second - first, third - first), first + second + third) > 0.0F ? 0 : 1;
    }
    BOOST_TEST(inward == 0U);
}

// a face-varying value equal at every corner of a vertex is smooth, as a vertex value is. One
// that differs on each face keeps each face's own values at its corners and between them
BOOST_AUTO_TEST_CASE(moya_subdivision_a_face_varying_value_keeps_its_seams_test) {
    const geometry::Topology box = cube();
    const geometry::Surface surface(box);
    const std::vector<glm::vec3> corners = cubePoints();
    const geometry::Tessellation cut = surface.cut(2);
    std::vector<glm::vec3> atCorners;
    atCorners.reserve(box.indices.size());
    for (const std::int32_t index : box.indices) {
        atCorners.push_back(corners[static_cast<std::size_t>(index)]);
    }
    const std::vector<float> smooth = surface.at(flat(atCorners), 3, Interpolation::FaceVarying, cut);
    const std::vector<float> limit = surface.at(flat(corners), 3, Interpolation::Vertex, cut);
    float apart = 0.0F;
    for (std::size_t at = 0; at < limit.size(); ++at) {
        apart = std::max(apart, std::fabs(smooth[at] - limit[at]));
    }
    BOOST_TEST(apart < 1.0e-5F);

    // each face runs from 0 to 1 in u and v from its first corner
    std::vector<float> uv;
    for (int face = 0; face < 6; ++face) {
        uv.insert(uv.end(), { 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F });
    }
    const std::vector<float> seamed = surface.at(uv, 2, Interpolation::FaceVarying,
        placesAt({ 0, 3, 3, 5 }, { glm::vec2(0.0F), glm::vec2(1.0F, 0.0F), glm::vec2(0.5F), glm::vec2(0.0F, 1.0F) }));
    const std::vector<float> want { 0.0F, 0.0F, 1.0F, 0.0F, 0.5F, 0.5F, 0.0F, 1.0F };
    for (std::size_t at = 0; at < want.size(); ++at) {
        BOOST_TEST(std::fabs(seamed[at] - want[at]) < 1.0e-5F, at);
    }
}

// a varying value runs bilinearly across each patch face, a uniform one is its face's, and a
// constant one is the same everywhere. Values of another count are refused
BOOST_AUTO_TEST_CASE(moya_subdivision_varying_uniform_and_constant_values_test) {
    const geometry::Surface surface(cube());
    const geometry::Tessellation places = placesAt({ 4, 4, 0 }, { glm::vec2(0.5F), glm::vec2(0.25F, 0.0F), glm::vec2(0.5F) });
    // the top's corners, 4, 5, 7 and 6, are 4, 5, 7 and 6
    const std::vector<float> varying = surface.at({ 0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F }, 1, Interpolation::Varying, places);
    BOOST_TEST(std::fabs(varying[0] - 5.5F) < 1.0e-5F);
    BOOST_TEST(std::fabs(varying[1] - 4.25F) < 1.0e-5F);
    const std::vector<float> uniform = surface.at({ 10.0F, 11.0F, 12.0F, 13.0F, 14.0F, 15.0F }, 1, Interpolation::Uniform, places);
    BOOST_TEST((uniform == std::vector<float> { 14.0F, 14.0F, 10.0F }));
    BOOST_TEST((surface.at({ 7.0F }, 1, Interpolation::Constant, places) == std::vector<float> { 7.0F, 7.0F, 7.0F }));
    BOOST_CHECK_THROW(surface.at({ 1.0F, 2.0F }, 1, Interpolation::Vertex, places), std::invalid_argument);
}

// a topology OpenSubdiv cannot refine is refused, and the scene's subdivision is left out and
// reported for it
BOOST_AUTO_TEST_CASE(moya_subdivision_a_topology_that_cannot_be_refined_is_reported_test) {
    geometry::Topology edge = cube();
    edge.vertices = { 2, 4, 4, 4, 4, 4, 2 };
    BOOST_CHECK_THROW(geometry::Surface { edge }, std::invalid_argument);
    geometry::Topology quads = cube();
    quads.scheme = geometry::Scheme::Loop;
    BOOST_CHECK_THROW(geometry::Surface { quads }, std::invalid_argument);
    geometry::Topology outside = cube();
    outside.indices[3] = 8;
    BOOST_CHECK_THROW(geometry::Surface { outside }, std::invalid_argument);
    geometry::Topology crease = cube();
    crease.creases = { 4, 5 };
    crease.creaseLengths = { 2 };
    BOOST_CHECK_THROW(geometry::Surface { crease }, std::invalid_argument);

    scene::Scene scene(scene::kinds());
    const scene::Handle cage = subdivision(scene, "cage", outside, cubePoints());
    instanced(scene, "cage", cage, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "subdivision cage is left out, because vertex 8 lies outside its 8 points"));
    BOOST_TEST(prototypes.geometries.empty());
}

// a subdivision surface of no faces places nothing and is not reported, however many instances
// place it
BOOST_AUTO_TEST_CASE(moya_subdivision_a_surface_of_no_faces_places_nothing_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle cage = scene.create("subdivision", "cage");
    instanced(scene, "cage", cage, {}, glm::mat4(1.0F));
    instanced(scene, "cage again", cage, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Flat flat = geometry::flatten(scene, {});
    BOOST_TEST(flat.reports.empty());
    BOOST_TEST(flat.faces.empty());
    BOOST_TEST(flat.placements.size() == 2U);
}

// a ray down onto the cube's top meets its limit at the top's middle, as the cube stands at the
// ray's time, through every traversal. The hit view names the face it was cut from
BOOST_AUTO_TEST_CASE(moya_subdivision_a_subdivision_surface_is_traced_test) {
    scene::Scene scene(scene::kinds());
    shutter(scene);
    const scene::Handle cage = subdivision(scene, "cage", cube(), cubePoints());
    std::vector<glm::vec3> raised = cubePoints();
    for (glm::vec3 & point : raised) {
        point.z += 1.0F;
    }
    scene.set(cage, "P", { 0.0F, 1.0F }, { scene::Value::points(cubePoints()), scene::Value::points(raised) });
    instanced(scene, "cage", cage, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.empty());
    std::vector<kernel::geometry::Ray> rays;
    for (const float time : { 0.0F, 0.5F }) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { 0.0F, 0.0F, 5.0F };
        ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
        ray.time = time;
        rays.push_back(ray);
    }
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, rays, tracer);
        for (std::size_t i = 0; i < rays.size(); ++i) {
            const float want = 5.0F - (68.0F / 81.0F) - rays[i].time;
            BOOST_TEST(std::fabs(traced.nearest[i].distance - want) < 1.0e-4F, tracerName(tracer, isa) << ", ray " << i);
        }
    });

    scene::Scene still(scene::kinds());
    const scene::Handle options = still.create("options", "options");
    const scene::Handle camera = still.create("camera", "camera");
    still.set(options, "resolution", scene::Value::integers({ 9, 9 }));
    still.set(options, "camera", scene::Value::handle(camera));
    still.set(camera, "projection", scene::Value::string("orthographic"));
    // the cube turned over about x, so that its top, face 4, faces the camera from z = 3
    const scene::Handle box = subdivision(still, "box", cube(), cubePoints());
    instanced(still, "box", box, {}, moved(0.0F, 0.0F, 3.0F) * glm::rotate(glm::mat4(1.0F), 3.14159265F, glm::vec3 { 1.0F, 0.0F, 0.0F }));
    still.commit();
    still.apply();
    CpuDevice device;
    const v3d::moya::film::Snapshot snapshot = view::hits(device, still, {}, 0.0F, view::Tracer::Hierarchy);
    BOOST_TEST(snapshot.at(4, 4, snapshot.offset(view::kFace)) == 4.0F);
    BOOST_TEST(std::fabs(snapshot.at(4, 4, snapshot.offset(view::kDepth)) - (3.0F - (68.0F / 81.0F))) < 1.0e-4F);
}

};  // namespace
};  // namespace v3d::moya::tracing
