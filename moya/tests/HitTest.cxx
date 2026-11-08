/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Offset.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/kernel/geometry/Triangle.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

namespace geometry = v3d::moya::kernel::geometry;

using Triangle = std::array<glm::vec3, 3>;

/** A traversal that gives the same answer for every ray. **/
struct Complete {
    geometry::Hit answer;

    geometry::Hit nearest(const geometry::Ray & /*ray*/) const {
        return answer;
    }
    bool any(const geometry::Ray & /*ray*/) const {
        return answer.found();
    }
};

/** A traversal with no test for any hit, which a shadow ray needs. **/
struct NearestOnly {
    geometry::Hit answer;

    geometry::Hit nearest(const geometry::Ray & /*ray*/) const {
        return answer;
    }
};

static_assert(geometry::Traversal<Complete>);
static_assert(!geometry::Traversal<NearestOnly>);

/** The faces of an octahedron about a centre, turned off the axes so no edge lies along one. **/
std::vector<Triangle> octahedron(const glm::vec3 & centre, float radius) {
    const glm::vec3 x = glm::normalize(glm::vec3 { 0.8F, 0.3F, -0.2F });
    const glm::vec3 y = glm::normalize(glm::cross(glm::vec3 { 0.1F, 0.4F, 0.9F }, x));
    const glm::vec3 z = glm::cross(x, y);
    const std::array<glm::vec3, 6> corners {
        centre + (x * radius), centre - (x * radius), centre + (y * radius),
        centre - (y * radius), centre + (z * radius), centre - (z * radius)
    };
    std::vector<Triangle> faces;
    for (int sx = 0; sx < 2; ++sx) {
        for (int sy = 2; sy < 4; ++sy) {
            for (int sz = 4; sz < 6; ++sz) {
                faces.push_back({ corners[sx], corners[sy], corners[sz] });
            }
        }
    }
    return faces;
}

/** Whether a ray meets any of the triangles. **/
bool meetsAny(const geometry::Ray & ray, const std::vector<Triangle> & faces) {
    const geometry::Shear shear = geometry::Shear::of(ray);
    for (const Triangle & face : faces) {
        geometry::TriangleHit hit;
        if (geometry::intersectTriangle(shear, face[0], face[1], face[2], ray.start, ray.end, hit)) {
            return true;
        }
    }
    return false;
}

// rays from inside a closed mesh towards points along every edge and at every vertex, where two
// or more triangles meet, each find one of them
BOOST_AUTO_TEST_CASE(moya_hit_a_ray_through_a_shared_edge_or_vertex_meets_the_mesh_test) {
    const glm::vec3 centre { 3.1F, -7.3F, 11.7F };
    const std::vector<Triangle> faces = octahedron(centre, 2.3F);
    int misses = 0;
    for (const Triangle & face : faces) {
        for (int edge = 0; edge < 3; ++edge) {
            const glm::vec3 & from = face[edge];
            const glm::vec3 & to = face[(edge + 1) % 3];
            for (int step = 0; step <= 64; ++step) {
                const float t = static_cast<float>(step) / 64.0F;
                const glm::vec3 target = from + ((to - from) * t);
                geometry::Ray ray;
                ray.origin = centre;
                ray.direction = target - centre;
                if (!meetsAny(ray, faces)) {
                    ++misses;
                }
            }
        }
    }
    BOOST_TEST(misses == 0);
}

BOOST_AUTO_TEST_CASE(moya_hit_a_triangle_is_met_only_within_the_interval_test) {
    const glm::vec3 p0 { -1.0F, -1.0F, -5.0F };
    const glm::vec3 p1 { 1.0F, -1.0F, -5.0F };
    const glm::vec3 p2 { 0.0F, 1.0F, -5.0F };
    geometry::Ray ray;
    geometry::TriangleHit hit;

    ray.end = 4.0F;
    BOOST_TEST(!geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit));

    ray.start = 6.0F;
    ray.end = 10.0F;
    BOOST_TEST(!geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit));

    ray.start = 0.0F;
    BOOST_TEST(geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit));
    BOOST_TEST(hit.distance == 5.0F);

    // the end of the interval is open, so a triangle at the end is not met
    ray.end = 5.0F;
    BOOST_TEST(!geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit));
}

BOOST_AUTO_TEST_CASE(moya_hit_a_triangle_gives_the_weights_of_its_second_and_third_vertices_test) {
    const glm::vec3 p0 { 0.0F, 0.0F, -2.0F };
    const glm::vec3 p1 { 4.0F, 0.0F, -2.0F };
    const glm::vec3 p2 { 0.0F, 4.0F, -2.0F };
    geometry::Ray ray;
    ray.origin = glm::vec3 { 1.0F, 2.0F, 0.0F };
    geometry::TriangleHit hit;
    BOOST_TEST(geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit));
    BOOST_TEST(hit.uv.x == 0.25F);
    BOOST_TEST(hit.uv.y == 0.5F);

    // either side is met, with the same weights
    ray.origin = glm::vec3 { 1.0F, 2.0F, -4.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
    BOOST_TEST(geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit));
    BOOST_TEST(hit.uv.x == 0.25F);
    BOOST_TEST(hit.uv.y == 0.5F);
}

// rays leaving a large triangle far from the origin, at every angle down to grazing, do not
// meet it again where they start
BOOST_AUTO_TEST_CASE(moya_hit_a_ray_offset_from_its_hit_does_not_meet_the_surface_again_test) {
    for (const float far : { 1.0F, 1000.0F, 100000.0F }) {
        const glm::vec3 p0 { far, far * 0.5F, -far };
        const glm::vec3 p1 { far * 3.0F, far * 0.7F, -far * 1.3F };
        const glm::vec3 p2 { far * 1.2F, far * 2.9F, -far * 0.6F };
        const glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
        const glm::vec3 unit = glm::normalize(normal);
        const glm::vec3 along = glm::normalize(p1 - p0);
        const glm::vec3 across = glm::cross(unit, along);

        int again = 0;
        for (int i = 1; i < 32; ++i) {
            for (int j = 1; j < 32 - i; ++j) {
                const float u = static_cast<float>(i) / 32.0F;
                const float v = static_cast<float>(j) / 32.0F;
                const glm::vec3 point = p0 + ((p1 - p0) * u) + ((p2 - p0) * v);
                for (int k = 0; k < 16; ++k) {
                    // from straight off the surface down to a few thousandths of a degree above it
                    const float lift = std::pow(10.0F, -static_cast<float>(k) / 3.0F);
                    const float turn = static_cast<float>(k) * 0.7F;
                    geometry::Ray ray;
                    ray.origin = geometry::offsetStart(point, unit);
                    ray.direction = (unit * lift) + (along * std::cos(turn)) + (across * std::sin(turn));
                    geometry::TriangleHit hit;
                    if (geometry::intersectTriangle(geometry::Shear::of(ray), p0, p1, p2, ray.start, ray.end, hit)) {
                        ++again;
                    }
                }
            }
        }
        BOOST_TEST(again == 0, "at " << far);
    }
}

BOOST_AUTO_TEST_CASE(moya_hit_a_hit_that_found_nothing_says_so_test) {
    const geometry::Hit none;
    BOOST_TEST(!none.found());
    BOOST_TEST(std::isinf(none.distance));

    geometry::Hit some;
    some.primitive = 0;
    BOOST_TEST(some.found());
}

};  // namespace
