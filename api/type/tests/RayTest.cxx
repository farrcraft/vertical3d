/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Plane.h>
#include <api/type/geometry/Ray.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

BOOST_AUTO_TEST_CASE(ray_direction_test) {
    // the constructor normalises, so a distance along the ray is in the units its origin
    // and direction were given in
    const v3d::type::geometry::Ray ray({.origin = glm::vec3(1.0F, 2.0F, 3.0F), .direction = glm::vec3(0.0F, 0.0F, 5.0F)});
    BOOST_CHECK_CLOSE(ray.direction()[2], 1.0F, 0.01F);
    BOOST_CHECK_EQUAL(ray.origin()[0], 1.0F);

    glm::vec3 along = ray.point(2.0F);
    BOOST_CHECK_CLOSE(along[2], 5.0F, 0.01F);
    BOOST_CHECK_CLOSE(along[0], 1.0F, 0.01F);

    // a zero direction is left alone rather than dividing by zero
    const v3d::type::geometry::Ray degenerate({.origin = glm::vec3(0.0F), .direction = glm::vec3(0.0F)});
    BOOST_CHECK_EQUAL(degenerate.direction()[0], 0.0F);
    BOOST_CHECK_EQUAL(degenerate.direction()[2], 0.0F);
}

BOOST_AUTO_TEST_CASE(ray_transformed_test) {
    const v3d::type::geometry::Ray ray({.origin = glm::vec3(0.0F, 0.0F, -10.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});

    // the origin moves as a point and the direction as a vector, so a translation leaves
    // the direction alone
    const glm::mat4 moved = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 3.0F, 0.0F));
    const v3d::type::geometry::Ray translated = ray.transformed(moved);
    BOOST_CHECK_CLOSE(translated.origin()[1], 3.0F, 0.01F);
    BOOST_CHECK_CLOSE(translated.direction()[2], 1.0F, 0.01F);

    // a scale is not renormalised away: the direction is halved, so the distance to a
    // given point is the same number it was in the space the ray came from
    const glm::mat4 halved = glm::scale(glm::mat4(1.0F), glm::vec3(0.5F, 0.5F, 0.5F));
    const v3d::type::geometry::Ray scaled = ray.transformed(halved);
    BOOST_CHECK_CLOSE(scaled.direction()[2], 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(scaled.origin()[2], -5.0F, 0.01F);
    BOOST_CHECK_CLOSE(scaled.point(10.0F)[2], 0.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(ray_triangle_test) {
    const glm::vec3 p0(-1.0F, -1.0F, 0.0F);
    const glm::vec3 p1(1.0F, -1.0F, 0.0F);
    const glm::vec3 p2(0.0F, 1.0F, 0.0F);

    float distance = 0.0F;

    // straight through the middle
    const v3d::type::geometry::Ray hit({.origin = glm::vec3(0.0F, 0.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(hit.intersects(p0, p1, p2, &distance), true);
    BOOST_CHECK_CLOSE(distance, 5.0F, 0.01F);

    // from behind, which a modeller picks as readily as the front
    const v3d::type::geometry::Ray behind({.origin = glm::vec3(0.0F, 0.0F, 5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    BOOST_CHECK_EQUAL(behind.intersects(p0, p1, p2, &distance), true);
    BOOST_CHECK_CLOSE(distance, 5.0F, 0.01F);

    // outside the triangle but in its plane's way
    const v3d::type::geometry::Ray beside({.origin = glm::vec3(3.0F, 0.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(beside.intersects(p0, p1, p2, nullptr), false);

    // pointing away from it - a half line, not a line
    const v3d::type::geometry::Ray away({.origin = glm::vec3(0.0F, 0.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    BOOST_CHECK_EQUAL(away.intersects(p0, p1, p2, nullptr), false);

    // edge on, where the barycentric solution is meaningless
    const v3d::type::geometry::Ray edgeOn({.origin = glm::vec3(0.0F, 0.0F, 0.0F), .direction = glm::vec3(1.0F, 0.0F, 0.0F)});
    BOOST_CHECK_EQUAL(edgeOn.intersects(p0, p1, p2, nullptr), false);
}

BOOST_AUTO_TEST_CASE(ray_triangle_barycentric_test) {
    // a right triangle on the z = 0 plane, so a weight reads straight off a coordinate
    const glm::vec3 p0(0.0F, 0.0F, 0.0F);
    const glm::vec3 p1(4.0F, 0.0F, 0.0F);
    const glm::vec3 p2(0.0F, 4.0F, 0.0F);

    float distance = 0.0F;
    glm::vec2 weights(0.0F);

    // a hit at p1 itself weighs p1 alone
    const v3d::type::geometry::Ray corner({.origin = glm::vec3(4.0F, 0.0F, -2.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(corner.intersects(p0, p1, p2, &distance, &weights), true);
    BOOST_CHECK_CLOSE(distance, 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(weights.x, 1.0F, 0.01F);
    BOOST_CHECK_SMALL(weights.y, 0.0001F);

    // (1, 2) is p0 + (1/4)(p1 - p0) + (1/2)(p2 - p0), so p0 carries the remaining quarter
    const v3d::type::geometry::Ray inside({.origin = glm::vec3(1.0F, 2.0F, -2.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(inside.intersects(p0, p1, p2, &distance, &weights), true);
    BOOST_CHECK_CLOSE(weights.x, 0.25F, 0.01F);
    BOOST_CHECK_CLOSE(weights.y, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(1.0F - weights.x - weights.y, 0.25F, 0.01F);

    // the weights are of p1 and p2 in that order, not of the two the ray happens to be
    // nearer: swapping the triangle's last two corners swaps them
    BOOST_CHECK_EQUAL(inside.intersects(p0, p2, p1, &distance, &weights), true);
    BOOST_CHECK_CLOSE(weights.x, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(weights.y, 0.25F, 0.01F);

    // a miss writes nothing, so what the caller had stands
    weights = glm::vec2(-1.0F);
    const v3d::type::geometry::Ray beside({.origin = glm::vec3(5.0F, 5.0F, -2.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(beside.intersects(p0, p1, p2, &distance, &weights), false);
    BOOST_CHECK_EQUAL(weights.x, -1.0F);
    BOOST_CHECK_EQUAL(weights.y, -1.0F);

    // the overload without them returns what the overload with them returns
    const v3d::type::geometry::Ray same({.origin = glm::vec3(1.0F, 2.0F, -2.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    float plain = 0.0F;
    BOOST_CHECK_EQUAL(same.intersects(p0, p1, p2, &plain), true);
    BOOST_CHECK_CLOSE(plain, 2.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(ray_box_test) {
    v3d::type::geometry::AABBox box;
    box.extents(glm::vec3(-1.0F, -1.0F, -1.0F), glm::vec3(1.0F, 1.0F, 1.0F));

    float distance = 0.0F;

    const v3d::type::geometry::Ray hit({.origin = glm::vec3(0.0F, 0.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(hit.intersects(box, &distance), true);
    BOOST_CHECK_CLOSE(distance, 4.0F, 0.01F);

    // a ray that starts inside hits at zero rather than missing
    const v3d::type::geometry::Ray inside({.origin = glm::vec3(0.0F, 0.0F, 0.0F), .direction = glm::vec3(0.0F, 1.0F, 0.0F)});
    BOOST_CHECK_EQUAL(inside.intersects(box, &distance), true);
    BOOST_CHECK_EQUAL(distance, 0.0F);

    // parallel to a pair of slabs and outside them
    const v3d::type::geometry::Ray parallel({.origin = glm::vec3(0.0F, 5.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK_EQUAL(parallel.intersects(box, nullptr), false);

    // pointing away
    const v3d::type::geometry::Ray away({.origin = glm::vec3(0.0F, 0.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    BOOST_CHECK_EQUAL(away.intersects(box, nullptr), false);
}

namespace {

/**
 * The ground at height y, facing up.
 **/
v3d::type::geometry::Plane ground(float y) {
    v3d::type::geometry::Plane plane;
    plane.calculate(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, y, 0.0F));
    return plane;
}

};  // namespace

BOOST_AUTO_TEST_CASE(ray_plane_test) {
    float distance = 0.0F;

    // straight down onto the ground lands directly below, as far away as the ground is
    const v3d::type::geometry::Ray down({.origin = glm::vec3(1.0F, 10.0F, 2.0F), .direction = glm::vec3(0.0F, -1.0F, 0.0F)});
    BOOST_TEST(down.intersects(ground(3.0F), &distance));
    BOOST_CHECK_CLOSE(distance, 7.0F, 0.01F);
    glm::vec3 hit = down.point(distance);
    BOOST_CHECK_CLOSE(hit[0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(hit[1], 3.0F, 0.01F);
    BOOST_CHECK_CLOSE(hit[2], 2.0F, 0.01F);

    // a ray running alongside the ground never reaches it, and neither does one lying in it
    const v3d::type::geometry::Ray parallel({.origin = glm::vec3(0.0F, 10.0F, 0.0F), .direction = glm::vec3(1.0F, 0.0F, 0.0F)});
    BOOST_TEST(!parallel.intersects(ground(3.0F), &distance));
    const v3d::type::geometry::Ray lying({.origin = glm::vec3(0.0F, 3.0F, 0.0F), .direction = glm::vec3(1.0F, 0.0F, 0.0F)});
    BOOST_TEST(!lying.intersects(ground(3.0F), &distance));

    // a ray pointing at the sky would have to run backwards to reach the ground
    const v3d::type::geometry::Ray up({.origin = glm::vec3(0.0F, 10.0F, 0.0F), .direction = glm::vec3(0.0F, 1.0F, 1.0F)});
    BOOST_TEST(!up.intersects(ground(3.0F), nullptr));

    // and one coming up from below crosses it from the other side
    const v3d::type::geometry::Ray under({.origin = glm::vec3(0.0F, -1.0F, 0.0F), .direction = glm::vec3(0.0F, 1.0F, 0.0F)});
    BOOST_TEST(under.intersects(ground(3.0F), &distance));
    BOOST_CHECK_CLOSE(distance, 4.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(ray_plane_unnormalised_test) {
    // a normal ten units long scales the plane's equation and the ray's approach to it alike,
    // so the crossing is where it would be for a unit normal
    v3d::type::geometry::Plane plane;
    plane[v3d::type::geometry::Plane::A] = 0.0F;
    plane[v3d::type::geometry::Plane::B] = 10.0F;
    plane[v3d::type::geometry::Plane::C] = 0.0F;
    plane[v3d::type::geometry::Plane::D] = -30.0F;

    const v3d::type::geometry::Ray slanted({.origin = glm::vec3(0.0F, 7.0F, 0.0F), .direction = glm::vec3(1.0F, -1.0F, 0.0F)});
    float distance = 0.0F;
    BOOST_TEST(slanted.intersects(plane, &distance));
    glm::vec3 hit = slanted.point(distance);
    BOOST_CHECK_CLOSE(hit[0], 4.0F, 0.01F);
    BOOST_CHECK_SMALL(hit[1] - 3.0F, 0.0001F);
}

/**
 * A ground pick through an orthographic camera looking down at an angle. Its rays are
 * parallel, so the two clicks land in different places on the ground. Two clicks a
 * horizontal step apart on the screen start the same height above it and travel the same
 * distance to reach it.
 **/
BOOST_AUTO_TEST_CASE(ray_plane_orthographic_pick_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(10.0F, 10.0F, 10.0F));
    camera.profile().up(glm::vec3(0.0F, 1.0F, 0.0F));
    camera.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };
    const v3d::type::geometry::Ray left = camera.ray(glm::vec2(200.0F, 240.0F), viewport);
    const v3d::type::geometry::Ray right = camera.ray(glm::vec2(440.0F, 240.0F), viewport);

    float leftDistance = 0.0F;
    float rightDistance = 0.0F;
    BOOST_TEST(left.intersects(ground(0.0F), &leftDistance));
    BOOST_TEST(right.intersects(ground(0.0F), &rightDistance));

    const glm::vec3 leftHit = left.point(leftDistance);
    const glm::vec3 rightHit = right.point(rightDistance);
    BOOST_CHECK_SMALL(leftHit[1], 0.001F);
    BOOST_CHECK_SMALL(rightHit[1], 0.001F);
    BOOST_CHECK_GT(glm::length(rightHit - leftHit), 0.1F);
    BOOST_CHECK_CLOSE(leftDistance, rightDistance, 0.01F);
}

/**
 * A ray that is not a number anywhere meets no box. Compared slab by slab it would pass every
 * test, because every comparison with NaN is false.
 **/
BOOST_AUTO_TEST_CASE(ray_that_is_not_a_number_misses_test) {
    v3d::type::geometry::AABBox box;
    box.extents(glm::vec3(-1.0F, -1.0F, -1.0F), glm::vec3(1.0F, 1.0F, 1.0F));

    const float nan = std::nanf("");
    const v3d::type::geometry::Ray lost({.origin = glm::vec3(0.0F, 0.0F, -5.0F), .direction = glm::vec3(nan, 0.0F, 1.0F)});
    BOOST_CHECK(!lost.intersects(box, nullptr));
    const v3d::type::geometry::Ray nowhere({.origin = glm::vec3(nan, 0.0F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    BOOST_CHECK(!nowhere.intersects(box, nullptr));
}

/**
 * A ray that is not a number anywhere meets no triangle and crosses no plane, and leaves the
 * distance it was given untouched.
 **/
BOOST_AUTO_TEST_CASE(ray_that_is_not_a_number_misses_triangle_and_plane_test) {
    const float nan = std::nanf("");
    const v3d::type::geometry::Ray lost({.origin = glm::vec3(0.25F, 0.25F, -5.0F), .direction = glm::vec3(nan, 0.0F, 1.0F)});
    const v3d::type::geometry::Ray nowhere({.origin = glm::vec3(nan, 0.25F, -5.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    const glm::vec3 p0(0.0F, 0.0F, 0.0F);
    const glm::vec3 p1(1.0F, 0.0F, 0.0F);
    const glm::vec3 p2(0.0F, 1.0F, 0.0F);
    const v3d::type::geometry::Plane facing(p0, p1, p2);

    for (const v3d::type::geometry::Ray & ray : {lost, nowhere}) {
        float distance = -1.0F;
        glm::vec2 weights(-1.0F);
        BOOST_CHECK(!ray.intersects(p0, p1, p2, &distance));
        BOOST_CHECK(!ray.intersects(p0, p1, p2, &distance, &weights));
        BOOST_CHECK(!ray.intersects(facing, &distance));
        BOOST_CHECK_EQUAL(distance, -1.0F);
        BOOST_CHECK_EQUAL(weights.x, -1.0F);
        BOOST_CHECK_EQUAL(weights.y, -1.0F);
    }
}

/**
 * Two triangles that share an edge leave no crack along it. A ray through a point on the shared
 * edge meets at least one of them, though rounding can put the point just outside both. The
 * corners are two micropolygons of a displaced grid, where a pixel's ray through the edge once
 * met neither and left a hole in the picture. The rays along the rest of the edge are checked
 * too.
 **/
BOOST_AUTO_TEST_CASE(ray_shared_edge_has_no_crack_test) {
    const glm::vec3 outerLow(0.149999946F, 0.506250024F, 4.99391031F);
    const glm::vec3 low(0.18749997F, 0.506250024F, 4.99961472F);
    const glm::vec3 high(0.18750003F, 0.534374952F, 4.99961472F);
    const glm::vec3 beyondLow(0.224999994F, 0.506250024F, 4.99846268F);
    const glm::vec3 beyondHigh(0.224999994F, 0.534375012F, 4.99846268F);
    const auto meets = [&](const v3d::type::geometry::Ray & ray) {
        float distance = 0.0F;
        return ray.intersects(outerLow, low, high, &distance) || ray.intersects(low, beyondLow, beyondHigh, &distance) ||
            ray.intersects(low, beyondHigh, high, &distance);
    };
    BOOST_TEST(meets(v3d::type::geometry::Ray({.origin = glm::vec3(0.1875F, 0.520833313F, 1.0F),
        .direction = glm::vec3(0.0F, 0.0F, 1.0F)})));
    unsigned int cracks = 0;
    for (unsigned int k = 1; k < 1000; k++) {
        const glm::vec3 onEdge = low + ((high - low) * (static_cast<float>(k) / 1000.0F));
        if (!meets(v3d::type::geometry::Ray({.origin = glm::vec3(onEdge.x, onEdge.y, 1.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)}))) {
            cracks++;
        }
    }
    BOOST_TEST(cracks == 0U);
}

};  // namespace
