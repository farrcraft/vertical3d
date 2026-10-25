/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/view/Triangulation.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A mesh of one face in the z = 0 plane, facing +z. The points are given counter-clockwise
 * seen from +z, and the face is built clockwise, as every face is.
 **/
v3d::brep::BRep oneFace(std::vector<glm::vec3> counterClockwise) {
    std::reverse(counterClockwise.begin(), counterClockwise.end());
    v3d::brep::BRep mesh;
    mesh.addFace(counterClockwise, glm::vec3(0.0F, 0.0F, 1.0F));
    return mesh;
}

/**
 * The area of a triangle of the mesh's vertices, signed by which way it faces along +z: positive
 * when it winds counter-clockwise seen from +z.
 **/
float area(const v3d::brep::BRep& mesh, const std::array<v3d::brep::Index, 3>& triangle) {
    const glm::vec3 first = mesh.vertex(triangle[0])->point();
    const glm::vec3 second = mesh.vertex(triangle[1])->point();
    const glm::vec3 third = mesh.vertex(triangle[2])->point();
    return glm::cross(second - first, third - first).z * 0.5F;
}

/**
 * How a face should come out: how many triangles, and the area they cover together.
 **/
struct Expected {
    std::size_t triangles;
    float area;
};

/**
 * Every triangle winds counter-clockwise seen from outside, and together they cover the face's
 * area exactly, with no triangle facing the other way.
 **/
void covers(const v3d::brep::BRep& mesh, const Expected& expected) {
    const std::size_t triangles = expected.triangles;
    const float whole = expected.area;
    const std::vector<std::array<v3d::brep::Index, 3>> cut = v3d::editor::triangulateFace(mesh, 0);
    BOOST_REQUIRE_EQUAL(cut.size(), triangles);
    float total = 0.0F;
    for (const std::array<v3d::brep::Index, 3>& triangle : cut) {
        const float part = area(mesh, triangle);
        BOOST_CHECK_GT(part, 0.0F);
        total += part;
    }
    BOOST_CHECK_CLOSE(total, whole, 0.001F);
}

BOOST_AUTO_TEST_CASE(triangulate_quad_test) {
    covers(oneFace({ glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F), glm::vec3(2.0F, 1.0F, 0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F) }), { .triangles = 2, .area = 2.0F });
}

BOOST_AUTO_TEST_CASE(triangulate_pentagon_test) {
    // a regular pentagon of radius one
    std::vector<glm::vec3> points;
    for (int corner = 0; corner < 5; corner++) {
        const float angle = 6.2831853F * static_cast<float>(corner) / 5.0F;
        points.emplace_back(std::cos(angle), std::sin(angle), 0.0F);
    }
    covers(oneFace(points), { .triangles = 3, .area = 2.3776413F });
}

BOOST_AUTO_TEST_CASE(triangulate_concave_face_test) {
    // an L of three unit squares, whose inner corner a fan from the first corner would cut
    // across, so a fan covers area outside the face and leaves part of it bare
    covers(oneFace({ glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F), glm::vec3(2.0F, 1.0F, 0.0F),
        glm::vec3(1.0F, 1.0F, 0.0F), glm::vec3(1.0F, 2.0F, 0.0F), glm::vec3(0.0F, 2.0F, 0.0F) }), { .triangles = 4, .area = 3.0F });

    // the same L started at its inner corner, where a fan's first triangle has no area
    covers(oneFace({ glm::vec3(1.0F, 1.0F, 0.0F), glm::vec3(1.0F, 2.0F, 0.0F), glm::vec3(0.0F, 2.0F, 0.0F),
        glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F), glm::vec3(2.0F, 1.0F, 0.0F) }), { .triangles = 4, .area = 3.0F });
}

BOOST_AUTO_TEST_CASE(triangulate_degenerate_face_test) {
    // a face whose points lie on a line has no ear, and is fanned rather than lost
    const v3d::brep::BRep line = oneFace({ glm::vec3(0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F),
        glm::vec3(3.0F, 0.0F, 0.0F) });
    BOOST_CHECK_EQUAL(v3d::editor::triangulateFace(line, 0).size(), 2U);
}

BOOST_AUTO_TEST_CASE(triangulate_flat_normals_test) {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const v3d::editor::Triangles flat = v3d::editor::triangulate(*cube, v3d::editor::Shading::Flat);

    // six faces of two triangles, each face with its own four corners
    BOOST_CHECK_EQUAL(flat.indices.size(), 36U);
    BOOST_CHECK_EQUAL(flat.positions.size(), 24U);
    BOOST_REQUIRE_EQUAL(flat.normals.size(), flat.positions.size());

    // every triangle faces the way its normal does, outward, which is counter-clockwise
    for (std::size_t index = 0; index < flat.indices.size(); index += 3) {
        const glm::vec3 first = flat.positions[flat.indices[index]];
        const glm::vec3 second = flat.positions[flat.indices[index + 1]];
        const glm::vec3 third = flat.positions[flat.indices[index + 2]];
        const glm::vec3 facing = glm::normalize(glm::cross(second - first, third - first));
        BOOST_CHECK_SMALL(glm::distance(facing, flat.normals[flat.indices[index]]), 0.0001F);
        // outward from the cube's centre
        BOOST_CHECK_GT(glm::dot(facing, (first + second + third) / 3.0F), 0.0F);
    }
}

BOOST_AUTO_TEST_CASE(triangulate_smooth_normals_test) {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const v3d::editor::Triangles smooth = v3d::editor::triangulate(*cube, v3d::editor::Shading::Smooth);

    // one copy of each corner, lit by the three faces that meet there, which points it
    // straight out from the centre
    BOOST_CHECK_EQUAL(smooth.positions.size(), 8U);
    BOOST_CHECK_EQUAL(smooth.indices.size(), 36U);
    for (std::size_t vertex = 0; vertex < smooth.positions.size(); vertex++) {
        BOOST_CHECK_SMALL(glm::distance(smooth.normals[vertex], glm::normalize(smooth.positions[vertex])), 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(triangulate_zero_normal_test) {
    // a face whose stored normal is zero, as an extrude's ring has, takes one from its points
    v3d::brep::BRep mesh;
    mesh.addFace({ glm::vec3(0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F) },
        glm::vec3(0.0F));
    const v3d::editor::Triangles flat = v3d::editor::triangulate(mesh, v3d::editor::Shading::Flat);
    BOOST_REQUIRE(!flat.normals.empty());
    BOOST_CHECK_SMALL(glm::distance(flat.normals[0], glm::vec3(0.0F, 0.0F, 1.0F)), 0.0001F);
}

};  // namespace
