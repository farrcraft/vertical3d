/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Plane.h>
#include <api/type/geometry/Ray.h>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

BOOST_AUTO_TEST_CASE(ray_direction_test) {
    // the constructor normalises, so a distance along the ray is in the units its origin
    // and direction were given in
    v3d::type::geometry::Ray ray(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    BOOST_CHECK_CLOSE(ray.direction()[2], 1.0f, 0.01f);
    BOOST_CHECK_EQUAL(ray.origin()[0], 1.0f);

    glm::vec3 along = ray.point(2.0f);
    BOOST_CHECK_CLOSE(along[2], 5.0f, 0.01f);
    BOOST_CHECK_CLOSE(along[0], 1.0f, 0.01f);

    // a zero direction is left alone rather than dividing by zero
    v3d::type::geometry::Ray degenerate(glm::vec3(0.0f), glm::vec3(0.0f));
    BOOST_CHECK_EQUAL(degenerate.direction()[0], 0.0f);
    BOOST_CHECK_EQUAL(degenerate.direction()[2], 0.0f);
}

BOOST_AUTO_TEST_CASE(ray_transformed_test) {
    v3d::type::geometry::Ray ray(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // the origin moves as a point and the direction as a vector, so a translation leaves
    // the direction alone
    glm::mat4 moved = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));
    v3d::type::geometry::Ray translated = ray.transformed(moved);
    BOOST_CHECK_CLOSE(translated.origin()[1], 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(translated.direction()[2], 1.0f, 0.01f);

    // a scale is not renormalised away: the direction is halved, so the distance to a
    // given point is the same number it was in the space the ray came from
    glm::mat4 halved = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    v3d::type::geometry::Ray scaled = ray.transformed(halved);
    BOOST_CHECK_CLOSE(scaled.direction()[2], 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(scaled.origin()[2], -5.0f, 0.01f);
    BOOST_CHECK_CLOSE(scaled.point(10.0f)[2], 0.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(ray_triangle_test) {
    const glm::vec3 a(-1.0f, -1.0f, 0.0f);
    const glm::vec3 b(1.0f, -1.0f, 0.0f);
    const glm::vec3 c(0.0f, 1.0f, 0.0f);

    float distance = 0.0f;

    // straight through the middle
    v3d::type::geometry::Ray hit(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(hit.intersects(a, b, c, &distance), true);
    BOOST_CHECK_CLOSE(distance, 5.0f, 0.01f);

    // from behind, which a modeller picks as readily as the front
    v3d::type::geometry::Ray behind(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(behind.intersects(a, b, c, &distance), true);
    BOOST_CHECK_CLOSE(distance, 5.0f, 0.01f);

    // outside the triangle but in its plane's way
    v3d::type::geometry::Ray beside(glm::vec3(3.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(beside.intersects(a, b, c, nullptr), false);

    // pointing away from it - a half line, not a line
    v3d::type::geometry::Ray away(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(away.intersects(a, b, c, nullptr), false);

    // edge on, where the barycentric solution is meaningless
    v3d::type::geometry::Ray edgeOn(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_CHECK_EQUAL(edgeOn.intersects(a, b, c, nullptr), false);
}

BOOST_AUTO_TEST_CASE(ray_triangle_barycentric_test) {
    // a right triangle on the z = 0 plane, so a weight reads straight off a coordinate
    const glm::vec3 a(0.0f, 0.0f, 0.0f);
    const glm::vec3 b(4.0f, 0.0f, 0.0f);
    const glm::vec3 c(0.0f, 4.0f, 0.0f);

    float distance = 0.0f;
    float u = 0.0f;
    float v = 0.0f;

    // a hit at b itself weighs b alone
    v3d::type::geometry::Ray corner(glm::vec3(4.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(corner.intersects(a, b, c, &distance, &u, &v), true);
    BOOST_CHECK_CLOSE(distance, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(u, 1.0f, 0.01f);
    BOOST_CHECK_SMALL(v, 0.0001f);

    // (1, 2) is a + (1/4)(b - a) + (1/2)(c - a), so a carries the remaining quarter
    v3d::type::geometry::Ray inside(glm::vec3(1.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(inside.intersects(a, b, c, &distance, &u, &v), true);
    BOOST_CHECK_CLOSE(u, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(v, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(1.0f - u - v, 0.25f, 0.01f);

    // the weights are of b and c in that order, not of the two the ray happens to be
    // nearer: swapping the triangle's last two corners swaps them
    BOOST_CHECK_EQUAL(inside.intersects(a, c, b, &distance, &u, &v), true);
    BOOST_CHECK_CLOSE(u, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(v, 0.25f, 0.01f);

    // a miss writes nothing, so what the caller had stands
    u = -1.0f;
    v = -1.0f;
    v3d::type::geometry::Ray beside(glm::vec3(5.0f, 5.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(beside.intersects(a, b, c, &distance, &u, &v), false);
    BOOST_CHECK_EQUAL(u, -1.0f);
    BOOST_CHECK_EQUAL(v, -1.0f);

    // the overload without them returns what the overload with them returns
    v3d::type::geometry::Ray same(glm::vec3(1.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    float plain = 0.0f;
    BOOST_CHECK_EQUAL(same.intersects(a, b, c, &plain), true);
    BOOST_CHECK_CLOSE(plain, 2.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(ray_box_test) {
    v3d::type::geometry::AABBox box;
    box.extents(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    float distance = 0.0f;

    v3d::type::geometry::Ray hit(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(hit.intersects(box, &distance), true);
    BOOST_CHECK_CLOSE(distance, 4.0f, 0.01f);

    // a ray that starts inside hits at zero rather than missing
    v3d::type::geometry::Ray inside(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    BOOST_CHECK_EQUAL(inside.intersects(box, &distance), true);
    BOOST_CHECK_EQUAL(distance, 0.0f);

    // parallel to a pair of slabs and outside them
    v3d::type::geometry::Ray parallel(glm::vec3(0.0f, 5.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(parallel.intersects(box, nullptr), false);

    // pointing away
    v3d::type::geometry::Ray away(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(away.intersects(box, nullptr), false);
}

namespace {

/**
 * The ground at height y, facing up.
 **/
v3d::type::geometry::Plane ground(float y) {
    v3d::type::geometry::Plane plane;
    plane.calculate(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, y, 0.0f));
    return plane;
}

};  // namespace

BOOST_AUTO_TEST_CASE(ray_plane_test) {
    float distance = 0.0f;

    // straight down onto the ground lands directly below, as far away as the ground is
    v3d::type::geometry::Ray down(glm::vec3(1.0f, 10.0f, 2.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    BOOST_TEST(down.intersects(ground(3.0f), &distance));
    BOOST_CHECK_CLOSE(distance, 7.0f, 0.01f);
    glm::vec3 hit = down.point(distance);
    BOOST_CHECK_CLOSE(hit[0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(hit[1], 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(hit[2], 2.0f, 0.01f);

    // a ray running alongside the ground never reaches it, and neither does one lying in it
    v3d::type::geometry::Ray parallel(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_TEST(!parallel.intersects(ground(3.0f), &distance));
    v3d::type::geometry::Ray lying(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_TEST(!lying.intersects(ground(3.0f), &distance));

    // a ray pointing at the sky would have to run backwards to reach the ground
    v3d::type::geometry::Ray up(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f));
    BOOST_TEST(!up.intersects(ground(3.0f), nullptr));

    // and one coming up from below crosses it from the other side
    v3d::type::geometry::Ray under(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    BOOST_TEST(under.intersects(ground(3.0f), &distance));
    BOOST_CHECK_CLOSE(distance, 4.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(ray_plane_unnormalised_test) {
    // a normal ten units long scales the plane's equation and the ray's approach to it alike,
    // so the crossing is where it would be for a unit normal
    v3d::type::geometry::Plane plane;
    plane[v3d::type::geometry::Plane::A] = 0.0f;
    plane[v3d::type::geometry::Plane::B] = 10.0f;
    plane[v3d::type::geometry::Plane::C] = 0.0f;
    plane[v3d::type::geometry::Plane::D] = -30.0f;

    v3d::type::geometry::Ray slanted(glm::vec3(0.0f, 7.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f));
    float distance = 0.0f;
    BOOST_TEST(slanted.intersects(plane, &distance));
    glm::vec3 hit = slanted.point(distance);
    BOOST_CHECK_CLOSE(hit[0], 4.0f, 0.01f);
    BOOST_CHECK_SMALL(hit[1] - 3.0f, 0.0001f);
}

/**
 * A ground pick through an orthographic camera looking down at an angle. Its rays are
 * parallel, so the two clicks land in different places on the ground, and two clicks a
 * horizontal step apart on the screen start the same height above it and travel the same
 * distance to reach it.
 **/
BOOST_AUTO_TEST_CASE(ray_plane_orthographic_pick_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(10.0f, 10.0f, 10.0f));
    camera.profile().up(glm::vec3(0.0f, 1.0f, 0.0f));
    camera.profile().lookat(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };
    v3d::type::geometry::Ray left = camera.ray(glm::vec2(200.0f, 240.0f), viewport);
    v3d::type::geometry::Ray right = camera.ray(glm::vec2(440.0f, 240.0f), viewport);

    float leftDistance = 0.0f;
    float rightDistance = 0.0f;
    BOOST_TEST(left.intersects(ground(0.0f), &leftDistance));
    BOOST_TEST(right.intersects(ground(0.0f), &rightDistance));

    const glm::vec3 leftHit = left.point(leftDistance);
    const glm::vec3 rightHit = right.point(rightDistance);
    BOOST_CHECK_SMALL(leftHit[1], 0.001f);
    BOOST_CHECK_SMALL(rightHit[1], 0.001f);
    BOOST_CHECK_GT(glm::length(rightHit - leftHit), 0.1f);
    BOOST_CHECK_CLOSE(leftDistance, rightDistance, 0.01f);
}
