/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/kernel/geometry/primitive/Shear.h>
#include <moya/libmoya/kernel/geometry/primitive/Triangle.h>
#include <moya/libmoya/kernel/geometry/primitive/TriangleHit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace geometry = v3d::moya::geometry;

/** The area of a triangle of the mesh. **/
float area(const std::vector<glm::vec3> & points, const std::vector<std::int32_t> & indices, const geometry::Triangles & triangles,
    std::size_t triangle) {
    const glm::vec3 & p0 = points[static_cast<std::size_t>(indices[triangles.corners[(triangle * 3) + 0]])];
    const glm::vec3 & p1 = points[static_cast<std::size_t>(indices[triangles.corners[(triangle * 3) + 1]])];
    const glm::vec3 & p2 = points[static_cast<std::size_t>(indices[triangles.corners[(triangle * 3) + 2]])];
    return 0.5F * glm::length(glm::cross(p1 - p0, p2 - p0));
}

/** Whether a point in the z = 0 plane lies inside a polygon in that plane, by its crossings. **/
bool insidePolygon(const std::vector<glm::vec3> & polygon, const glm::vec2 & point) {
    bool in = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const glm::vec3 & p0 = polygon[i];
        const glm::vec3 & p1 = polygon[j];
        if ((p0.y > point.y) != (p1.y > point.y) && point.x < (((p1.x - p0.x) * (point.y - p0.y)) / (p1.y - p0.y)) + p0.x) {
            in = !in;
        }
    }
    return in;
}

/** An arrow pointing right, concave at its tail, anticlockwise in the z = 0 plane, of area 11. **/
std::vector<glm::vec3> arrow() {
    return { { 0.0F, 0.0F, 0.0F }, { 2.0F, 1.0F, 0.0F }, { 4.0F, 0.0F, 0.0F }, { 3.0F, 2.5F, 0.0F },
        { 4.0F, 5.0F, 0.0F }, { 2.0F, 4.0F, 0.0F }, { 0.0F, 5.0F, 0.0F }, { 1.0F, 2.5F, 0.0F } };
}

// a concave face's triangles add up to its area, and each one's centre lies inside it, so they
// cover the face and nothing outside it
BOOST_AUTO_TEST_CASE(moya_triangulate_a_concave_face_is_cut_inside_itself_test) {
    const std::vector<glm::vec3> points = arrow();
    std::vector<std::int32_t> indices;
    indices.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        indices.push_back(static_cast<std::int32_t>(i));
    }
    const geometry::Triangles triangles = geometry::triangulate(points, { static_cast<std::int32_t>(points.size()) }, indices);
    BOOST_TEST(triangles.faces.size() == points.size() - 2);
    float total = 0.0F;
    for (std::size_t t = 0; t < triangles.faces.size(); ++t) {
        total += area(points, indices, triangles, t);
        glm::vec3 centre { 0.0F };
        for (std::size_t k = 0; k < 3; ++k) {
            centre += points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + k]])];
        }
        centre /= 3.0F;
        BOOST_TEST(insidePolygon(points, { centre.x, centre.y }), "triangle " << t);
    }
    BOOST_TEST(std::fabs(total - 11.0F) < 1.0e-5F);
}

// the same face, turned clockwise and stood on its side, is cut as well, each triangle keeping
// the face's winding
BOOST_AUTO_TEST_CASE(moya_triangulate_a_triangle_keeps_its_face_winding_test) {
    std::vector<glm::vec3> points;
    for (const glm::vec3 & point : arrow()) {
        points.emplace_back(0.0F, point.y, point.x);
    }
    std::vector<std::int32_t> indices;
    indices.reserve(points.size());
    for (std::size_t i = points.size(); i > 0; --i) {
        indices.push_back(static_cast<std::int32_t>(i - 1));
    }
    const geometry::Triangles triangles = geometry::triangulate(points, { static_cast<std::int32_t>(points.size()) }, indices);
    BOOST_TEST(triangles.faces.size() == points.size() - 2);
    float total = 0.0F;
    for (std::size_t t = 0; t < triangles.faces.size(); ++t) {
        const glm::vec3 & p0 = points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + 0]])];
        const glm::vec3 & p1 = points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + 1]])];
        const glm::vec3 & p2 = points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + 2]])];
        // reversed, the face runs anticlockwise seen from +x, so each normal points down +x
        BOOST_TEST(glm::cross(p1 - p0, p2 - p0).x > 0.0F, "triangle " << t);
        total += area(points, indices, triangles, t);
    }
    BOOST_TEST(std::fabs(total - 11.0F) < 1.0e-5F);
}

// a ray through any triangle of a face names that face, and a face-varying value is read at the
// triangle's corners
BOOST_AUTO_TEST_CASE(moya_triangulate_a_hit_on_any_triangle_of_a_face_names_the_face_test) {
    // a triangle, then a square, then a pentagon, side by side along x at z = -1
    const std::vector<glm::vec3> points { { 0.0F, 0.0F, -1.0F }, { 1.0F, 0.0F, -1.0F }, { 0.5F, 1.0F, -1.0F },
        { 2.0F, 0.0F, -1.0F }, { 3.0F, 0.0F, -1.0F }, { 3.0F, 1.0F, -1.0F }, { 2.0F, 1.0F, -1.0F },
        { 4.0F, 0.0F, -1.0F }, { 5.0F, 0.0F, -1.0F }, { 5.5F, 0.6F, -1.0F }, { 4.5F, 1.0F, -1.0F }, { 3.6F, 0.6F, -1.0F } };
    const std::vector<std::int32_t> vertices { 3, 4, 5 };
    const std::vector<std::int32_t> indices { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    const geometry::Triangles triangles = geometry::triangulate(points, vertices, indices);
    BOOST_TEST(triangles.faces.size() == 6U);

    const std::vector<float> xs { 0.5F, 2.2F, 2.8F, 4.0F, 4.5F, 5.2F };
    const std::vector<std::uint32_t> expected { 0, 1, 1, 2, 2, 2 };
    for (std::size_t i = 0; i < xs.size(); ++i) {
        v3d::moya::kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { xs[i], 0.4F, 0.0F };
        const auto shear = v3d::moya::kernel::geometry::primitive::Shear::of(ray);
        std::uint32_t named = 99;
        for (std::size_t t = 0; t < triangles.faces.size(); ++t) {
            v3d::moya::kernel::geometry::primitive::TriangleHit hit;
            const glm::vec3 & p0 = points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + 0]])];
            const glm::vec3 & p1 = points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + 1]])];
            const glm::vec3 & p2 = points[static_cast<std::size_t>(indices[triangles.corners[(t * 3) + 2]])];
            if (v3d::moya::kernel::geometry::primitive::intersectTriangle(shear, p0, p1, p2, ray.start, ray.end, hit)) {
                named = triangles.faces[t];
            }
        }
        BOOST_TEST(named == expected[i], "at x " << xs[i]);
    }
    // every corner of the pentagon's triangles is one of the pentagon's own five
    for (std::size_t t = 0; t < triangles.faces.size(); ++t) {
        if (triangles.faces[t] == 2U) {
            for (std::size_t k = 0; k < 3; ++k) {
                BOOST_TEST(triangles.corners[(t * 3) + k] >= 7U);
                BOOST_TEST(triangles.corners[(t * 3) + k] < 12U);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_triangulate_a_face_that_covers_no_area_is_dropped_test) {
    const std::vector<glm::vec3> points { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    // two vertices, three in a line, four in a line, and one good triangle
    const std::vector<std::int32_t> vertices { 2, 3, 4, 3 };
    const std::vector<std::int32_t> indices { 0, 1, 0, 1, 2, 0, 1, 2, 1, 0, 1, 3 };
    const geometry::Triangles triangles = geometry::triangulate(points, vertices, indices);
    BOOST_TEST(triangles.dropped == 3U);
    BOOST_TEST(triangles.faces.size() == 1U);
    BOOST_TEST(triangles.faces[0] == 3U);
}

// a corner in line with its neighbours is left out, and a quad with one is still two triangles
BOOST_AUTO_TEST_CASE(moya_triangulate_a_corner_in_line_adds_no_triangle_test) {
    const std::vector<glm::vec3> points { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F },
        { 2.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    const geometry::Triangles triangles = geometry::triangulate(points, { 5 }, { 0, 1, 2, 3, 4 });
    BOOST_TEST(triangles.faces.size() == 2U);
    BOOST_TEST(triangles.dropped == 0U);
}

BOOST_AUTO_TEST_CASE(moya_triangulate_counts_that_do_not_fit_are_refused_test) {
    const std::vector<glm::vec3> points { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    BOOST_CHECK_THROW(geometry::triangulate(points, { 4 }, { 0, 1, 2 }), std::invalid_argument);
    BOOST_CHECK_THROW(geometry::triangulate(points, { 3 }, { 0, 1, 2, 0 }), std::invalid_argument);
    BOOST_CHECK_THROW(geometry::triangulate(points, { 3 }, { 0, 1, 3 }), std::invalid_argument);
    BOOST_CHECK_THROW(geometry::triangulate(points, { -1 }, {}), std::invalid_argument);
}

};  // namespace
