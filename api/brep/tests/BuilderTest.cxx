/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Builder.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

using v3d::brep::BuildOptions;
using v3d::brep::Built;
using v3d::brep::Polygons;
using v3d::brep::Winding;

/**
 * Add a face whose corners each have a point of their own, as a file that splits its vertices
 * holds them.
 **/
void addFace(Polygons* polygons, const std::vector<glm::vec3>& corners) {
    polygons->counts.push_back(static_cast<std::uint32_t>(corners.size()));
    for (const glm::vec3& corner : corners) {
        polygons->indices.push_back(static_cast<std::uint32_t>(polygons->points.size()));
        polygons->points.push_back(corner);
    }
}

/**
 * The six faces of a unit cube centred on the origin, each counter-clockwise seen from outside,
 * either whole or each cut into two triangles across its diagonal.
 **/
Polygons cube(bool triangles) {
    const std::array<std::array<glm::vec3, 3>, 6> sides{ {
        // the normal, and two directions along the face whose cross product is the normal
        { glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1) },
        { glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0) },
        { glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 0) },
        { glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1) },
        { glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) },
        { glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0) },
    } };
    Polygons polygons;
    for (const std::array<glm::vec3, 3>& side : sides) {
        const glm::vec3 centre = side[0] * 0.5F;
        const glm::vec3 u = side[1] * 0.5F;
        const glm::vec3 v = side[2] * 0.5F;
        const std::array<glm::vec3, 4> quad{ centre - u - v, centre + u - v, centre + u + v, centre - u + v };
        if (triangles) {
            addFace(&polygons, { quad[0], quad[1], quad[2] });
            addFace(&polygons, { quad[0], quad[2], quad[3] });
        } else {
            addFace(&polygons, { quad[0], quad[1], quad[2], quad[3] });
        }
    }
    return polygons;
}

/**
 * A flat grid of unit squares in the xy plane, counter-clockwise seen from +z, sharing their
 * points, either as quads or each cut into two triangles.
 **/
Polygons grid(unsigned int across, bool triangles) {
    Polygons polygons;
    for (unsigned int y = 0; y <= across; y++) {
        for (unsigned int x = 0; x <= across; x++) {
            polygons.points.push_back(glm::vec3(static_cast<float>(x), static_cast<float>(y), 0.0F));
        }
    }
    const auto at = [across](unsigned int x, unsigned int y) { return (y * (across + 1)) + x; };
    for (unsigned int y = 0; y < across; y++) {
        for (unsigned int x = 0; x < across; x++) {
            const std::array<std::uint32_t, 4> quad{ at(x, y), at(x + 1, y), at(x + 1, y + 1), at(x, y + 1) };
            if (triangles) {
                polygons.counts.insert(polygons.counts.end(), { 3, 3 });
                polygons.indices.insert(polygons.indices.end(), { quad[0], quad[1], quad[2], quad[0], quad[2], quad[3] });
            } else {
                polygons.counts.push_back(4);
                polygons.indices.insert(polygons.indices.end(), quad.begin(), quad.end());
            }
        }
    }
    return polygons;
}

BuildOptions counterClockwise(bool join) {
    BuildOptions options;
    options.winding = Winding::CounterClockwise;
    options.joinTriangles = join;
    return options;
}

/**
 * Whether every face of a closed mesh around the origin faces away from it, and every half
 * edge has a pair.
 **/
void checkClosedOutward(const v3d::brep::BRep& mesh) {
    std::string problem;
    BOOST_CHECK_MESSAGE(mesh.validateStrict(&problem), problem);
    for (v3d::brep::Index edge = 0; edge < mesh.edgeCount(); edge++) {
        BOOST_CHECK(mesh.edge(edge)->pair() != v3d::brep::INVALID_ID);
    }
    for (v3d::brep::Index face = 0; face < mesh.faceCount(); face++) {
        BOOST_CHECK_GT(glm::dot(mesh.face(face)->normal(), v3d::brep::center(mesh, face)), 0.0F);
    }
}

BOOST_AUTO_TEST_CASE(builder_cube_in_triangles_builds_six_quads_test) {
    const Built built = v3d::brep::build(cube(true), counterClockwise(true));

    BOOST_REQUIRE_MESSAGE(built.mesh, built.problem);
    BOOST_CHECK_EQUAL(built.mesh->faceCount(), 6U);
    BOOST_CHECK_EQUAL(built.mesh->vertexCount(), 8U);
    BOOST_CHECK_EQUAL(built.mesh->edgeCount(), 24U);
    for (v3d::brep::Index face = 0; face < built.mesh->faceCount(); face++) {
        BOOST_CHECK_EQUAL(v3d::brep::faceLoop(*built.mesh, face).size(), 4U);
    }
    checkClosedOutward(*built.mesh);
}

BOOST_AUTO_TEST_CASE(builder_keeps_triangles_unless_asked_to_join_test) {
    const Built built = v3d::brep::build(cube(true), counterClockwise(false));

    BOOST_REQUIRE_MESSAGE(built.mesh, built.problem);
    BOOST_CHECK_EQUAL(built.mesh->faceCount(), 12U);
    BOOST_CHECK_EQUAL(built.mesh->vertexCount(), 8U);
    checkClosedOutward(*built.mesh);
}

BOOST_AUTO_TEST_CASE(builder_takes_either_winding_test) {
    // the same cube with every face's corners reversed runs clockwise seen from outside
    Polygons clockwise = cube(false);
    for (std::size_t face = 0; face < clockwise.counts.size(); face++) {
        std::reverse(clockwise.indices.begin() + static_cast<std::ptrdiff_t>(face * 4),
            clockwise.indices.begin() + static_cast<std::ptrdiff_t>((face * 4) + 4));
    }

    const Built fromCounter = v3d::brep::build(cube(false), counterClockwise(false));
    const Built fromClockwise = v3d::brep::build(clockwise, BuildOptions());

    BOOST_REQUIRE_MESSAGE(fromCounter.mesh, fromCounter.problem);
    BOOST_REQUIRE_MESSAGE(fromClockwise.mesh, fromClockwise.problem);
    checkClosedOutward(*fromCounter.mesh);
    checkClosedOutward(*fromClockwise.mesh);
}

BOOST_AUTO_TEST_CASE(builder_joins_a_grid_into_its_squares_test) {
    // every square's diagonal joins its two triangles, and no side joins two squares
    const Built built = v3d::brep::build(grid(3, true), counterClockwise(true));

    BOOST_REQUIRE_MESSAGE(built.mesh, built.problem);
    BOOST_CHECK_EQUAL(built.mesh->faceCount(), 9U);
    BOOST_CHECK_EQUAL(built.mesh->vertexCount(), 16U);
    std::string problem;
    BOOST_CHECK_MESSAGE(built.mesh->validateStrict(&problem), problem);
    for (v3d::brep::Index face = 0; face < built.mesh->faceCount(); face++) {
        BOOST_CHECK_EQUAL(v3d::brep::faceLoop(*built.mesh, face).size(), 4U);
        BOOST_CHECK_CLOSE(built.mesh->face(face)->normal().z, 1.0F, 0.001F);
    }
}

BOOST_AUTO_TEST_CASE(builder_joins_only_within_the_angle_test) {
    // two triangles on the diagonal from the origin to (1, 1, 0), folded square to each other
    Polygons folded;
    addFace(&folded, { glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0) });
    addFace(&folded, { glm::vec3(0, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0.5F, 0.5F, 0.7071F) });

    const Built kept = v3d::brep::build(folded, counterClockwise(true));
    BuildOptions wide = counterClockwise(true);
    wide.joinAngle = 2.0F;
    const Built joined = v3d::brep::build(folded, wide);

    BOOST_REQUIRE_MESSAGE(kept.mesh, kept.problem);
    BOOST_REQUIRE_MESSAGE(joined.mesh, joined.problem);
    BOOST_CHECK_EQUAL(kept.mesh->faceCount(), 2U);
    BOOST_CHECK_EQUAL(joined.mesh->faceCount(), 1U);
}

BOOST_AUTO_TEST_CASE(builder_refuses_three_faces_on_one_edge_test) {
    // the first two would join across the edge from start to end, which the third also runs along
    const glm::vec3 start(0, 0, 0);
    const glm::vec3 end(1, 0, 0);
    Polygons fin;
    addFace(&fin, { start, end, glm::vec3(0.5F, 0.4F, 0) });
    addFace(&fin, { end, start, glm::vec3(0.5F, -0.4F, 0) });
    addFace(&fin, { start, end, glm::vec3(0.5F, 0, 0.4F) });

    for (const bool join : { false, true }) {
        const Built built = v3d::brep::build(fin, counterClockwise(join));
        BOOST_CHECK(!built.mesh);
        BOOST_CHECK_NE(built.problem.find("more than two faces"), std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(builder_refuses_faces_wound_against_each_other_test) {
    const glm::vec3 start(0, 0, 0);
    const glm::vec3 end(1, 0, 0);
    Polygons flipped;
    addFace(&flipped, { start, end, glm::vec3(0.5F, 1, 0) });
    addFace(&flipped, { start, end, glm::vec3(0.5F, -1, 0) });

    const Built built = v3d::brep::build(flipped, counterClockwise(false));
    BOOST_CHECK(!built.mesh);
    BOOST_CHECK(!built.problem.empty());
}

BOOST_AUTO_TEST_CASE(builder_refuses_malformed_input_test) {
    Polygons counts = cube(false);
    counts.counts.back() = 5;
    Polygons index = cube(false);
    index.indices.back() = static_cast<std::uint32_t>(index.points.size());
    Polygons infinite = cube(false);
    infinite.points[3].y = std::numeric_limits<float>::infinity();
    Polygons twice;
    twice.points = { glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0) };
    twice.counts = { 4 };
    twice.indices = { 0, 1, 0, 3 };

    for (const Polygons& polygons : { counts, index, infinite, twice }) {
        const Built built = v3d::brep::build(polygons, counterClockwise(false));
        BOOST_CHECK(!built.mesh);
        BOOST_CHECK(!built.problem.empty());
    }
}

BOOST_AUTO_TEST_CASE(builder_welds_points_and_skips_collapsed_faces_test) {
    Polygons polygons;
    // -0 and 0 are one position
    addFace(&polygons, { glm::vec3(-0.0F, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) });
    addFace(&polygons, { glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(-1, 0, 0) });
    // a triangle collapsed to a line
    addFace(&polygons, { glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) });

    const Built built = v3d::brep::build(polygons, counterClockwise(false));

    BOOST_REQUIRE_MESSAGE(built.mesh, built.problem);
    BOOST_CHECK_EQUAL(built.mesh->faceCount(), 2U);
    BOOST_CHECK_EQUAL(built.mesh->vertexCount(), 4U);
    BOOST_CHECK_EQUAL(built.skipped, 1U);
}

BOOST_AUTO_TEST_CASE(builder_builds_a_large_mesh_quickly_test) {
    // 80,000 triangles, which build in under two seconds in a debug build. Pairing each half
    // edge by searching the others would take minutes
    const Polygons triangles = grid(200, true);

    const auto start = std::chrono::steady_clock::now();
    const Built built = v3d::brep::build(triangles, counterClockwise(true));
    const auto elapsed = std::chrono::steady_clock::now() - start;

    BOOST_REQUIRE_MESSAGE(built.mesh, built.problem);
    BOOST_CHECK_EQUAL(built.mesh->faceCount(), 40000U);
    BOOST_CHECK_EQUAL(built.mesh->vertexCount(), 201U * 201U);
    BOOST_CHECK_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 20);
}

};  // namespace
