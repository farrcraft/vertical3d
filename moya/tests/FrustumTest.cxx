/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Frustum.h>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

v3d::type::geometry::AABBox box(const glm::vec3 & min, const glm::vec3 & max) {
    v3d::type::geometry::AABBox b;
    b.extents(min, max);
    return b;
}

/**
 * A symmetric orthographic volume two units on a side, which is the shape the render
 * context's own projection builds.
 **/
glm::mat4x4 volume() {
    return glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
}

};  // namespace

/**
 * A box inside every half space is inside the frustum.
 **/
BOOST_AUTO_TEST_CASE(frustum_inside_test) {
    v3d::moya::Frustum frustum(volume());

    BOOST_TEST(frustum.intersect(box(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, 0.5f, 0.5f))) == v3d::moya::Frustum::INSIDE);
}

/**
 * The half spaces are intersected, not unioned: a box excluded by one plane is out of the
 * frustum whatever the other five say. Requiring all six to exclude it is what made the cull
 * in the first pass keep everything.
 **/
BOOST_AUTO_TEST_CASE(frustum_outside_one_plane_test) {
    v3d::moya::Frustum frustum(volume());

    // well off to the right, and so inside the left, top, bottom, near and far planes
    BOOST_TEST(frustum.intersect(box(glm::vec3(10.0f, -0.5f, -0.5f), glm::vec3(20.0f, 0.5f, 0.5f))) == v3d::moya::Frustum::OUTSIDE);

    // and the same off to the left
    BOOST_TEST(frustum.intersect(box(glm::vec3(-20.0f, -0.5f, -0.5f), glm::vec3(-10.0f, 0.5f, 0.5f))) == v3d::moya::Frustum::OUTSIDE);
}

/**
 * A box straddling a plane is neither in nor out.
 **/
BOOST_AUTO_TEST_CASE(frustum_crossing_test) {
    v3d::moya::Frustum frustum(volume());

    BOOST_TEST(frustum.intersect(box(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(4.0f, 0.5f, 0.5f))) == v3d::moya::Frustum::CROSSING);
}
