/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/Listener.h>

#include <limits>
#include <optional>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    BOOST_TEST_CONTEXT("expected (" << expected.x << ", " << expected.y << ", " << expected.z << ")") {
        BOOST_CHECK_SMALL(actual.x - expected.x, 1.0e-5F);
        BOOST_CHECK_SMALL(actual.y - expected.y, 1.0e-5F);
        BOOST_CHECK_SMALL(actual.z - expected.z, 1.0e-5F);
    }
}

/**
 * A listener built as glm::lookAt builds a view, with its right at forward x up.
 **/
v3d::audio::Listener facing(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
    v3d::audio::Listener listener;
    listener.position = position;
    listener.forward = forward;
    listener.up = up;
    listener.right = glm::cross(forward, up);
    return listener;
}

/**
 * A listener built as a camera of the other hand builds one, with its right at up x forward.
 **/
v3d::audio::Listener mirror(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
    v3d::audio::Listener listener = facing(position, forward, up);
    listener.right = -listener.right;
    return listener;
}

BOOST_AUTO_TEST_SUITE(listener_test)

/**
 * The default listener stands where the mixer's does, so a point in the world is heard where it
 * is.
 **/
BOOST_AUTO_TEST_CASE(the_default_listener_hears_the_world_as_it_is) {
    const v3d::audio::Listener listener;
    checkVector(v3d::audio::relative(listener, {3.0F, -2.0F, 5.0F}), {3.0F, -2.0F, 5.0F});
}

/**
 * A point is heard by where it is from the listener: what is ahead is at -z, what is to the
 * right is at +x, and what is above is at +y, however the listener stands and turns.
 **/
BOOST_AUTO_TEST_CASE(a_point_is_heard_from_where_the_listener_stands) {
    // moved and facing the default way, a point two ahead of it is heard two ahead
    const v3d::audio::Listener moved = facing({10.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F});
    checkVector(v3d::audio::relative(moved, {10.0F, 0.0F, -2.0F}), {0.0F, 0.0F, -2.0F});

    // turned to face +x, its right is +z, so the axes of the world and the mixer differ
    const v3d::audio::Listener turned = facing({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    checkVector(v3d::audio::relative(turned, {5.0F, 0.0F, 0.0F}), {0.0F, 0.0F, -5.0F});
    checkVector(v3d::audio::relative(turned, {0.0F, 0.0F, 3.0F}), {3.0F, 0.0F, 0.0F});
    checkVector(v3d::audio::relative(turned, {-4.0F, 0.0F, 0.0F}), {0.0F, 0.0F, 4.0F});

    // lying on its back, looking up, its up is -z and what is above the world is ahead of it
    const v3d::audio::Listener lying = facing({0.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, -1.0F});
    checkVector(v3d::audio::relative(lying, {0.0F, 3.0F, 0.0F}), {0.0F, 0.0F, -2.0F});
    checkVector(v3d::audio::relative(lying, {0.0F, 1.0F, -1.0F}), {0.0F, 1.0F, 0.0F});
    checkVector(v3d::audio::relative(lying, {1.0F, 1.0F, 0.0F}), {-1.0F, 0.0F, 0.0F});
}

/**
 * A camera of the other hand puts up x forward on the right of the screen. A listener built from
 * it hears a point there on the right, though forward and up are the same as a glm::lookAt
 * camera's. Taking right from forward and up alone would put every sound on the wrong side.
 **/
BOOST_AUTO_TEST_CASE(a_mirrored_listener_hears_its_own_right) {
    const v3d::audio::Listener plain = facing({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F});
    const v3d::audio::Listener mirrored = mirror({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F});
    BOOST_TEST(!v3d::audio::mirrored(plain));
    BOOST_TEST(v3d::audio::mirrored(mirrored));

    checkVector(v3d::audio::relative(plain, {2.0F, 0.0F, 0.0F}), {2.0F, 0.0F, 0.0F});
    checkVector(v3d::audio::relative(mirrored, {2.0F, 0.0F, 0.0F}), {-2.0F, 0.0F, 0.0F});
    checkVector(v3d::audio::relative(mirrored, {-2.0F, 1.0F, -3.0F}), {2.0F, 1.0F, -3.0F});
}

/**
 * A forward of any length keeps its direction and is made unit length, and an up that leans
 * along it is stood upright, so a game can hand over a camera's vectors as they are. A right
 * that leans keeps only its side, in either hand.
 **/
BOOST_AUTO_TEST_CASE(a_listener_is_made_orthonormal) {
    v3d::audio::Listener given = facing({1.0F, 2.0F, 3.0F}, {0.0F, 0.0F, -4.0F}, {0.0F, 2.0F, -2.0F});
    given.right = glm::vec3(3.0F, 1.0F, 1.0F);
    const std::optional<v3d::audio::Listener> made = v3d::audio::orthonormal(given);
    BOOST_REQUIRE(made.has_value());
    const v3d::audio::Listener listener = made.value_or(v3d::audio::Listener());
    checkVector(listener.position, {1.0F, 2.0F, 3.0F});
    checkVector(listener.forward, {0.0F, 0.0F, -1.0F});
    checkVector(listener.up, {0.0F, 1.0F, 0.0F});
    checkVector(listener.right, {1.0F, 0.0F, 0.0F});

    given.right = glm::vec3(-0.5F, 3.0F, 0.0F);
    const std::optional<v3d::audio::Listener> left = v3d::audio::orthonormal(given);
    BOOST_REQUIRE(left.has_value());
    checkVector(left.value_or(v3d::audio::Listener()).right, {-1.0F, 0.0F, 0.0F});
}

/**
 * A listener that faces no way is refused: a forward or an up of zero, the two parallel, a
 * right of zero or one in their plane, or a value that is not a number. Each would otherwise
 * place every sound at a point of noise, or on a side picked by rounding.
 **/
BOOST_AUTO_TEST_CASE(a_listener_that_faces_no_way_is_refused) {
    const glm::vec3 origin(0.0F);
    const glm::vec3 ahead(0.0F, 0.0F, -1.0F);
    const glm::vec3 up(0.0F, 1.0F, 0.0F);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();

    BOOST_TEST(v3d::audio::orthonormal(facing(origin, ahead, up)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing(origin, origin, up)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing(origin, ahead, origin)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing(origin, ahead, ahead * 3.0F)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing(origin, ahead, -ahead)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing({nan, 0.0F, 0.0F}, ahead, up)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing({infinity, 0.0F, 0.0F}, ahead, up)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing(origin, {0.0F, nan, -1.0F}, up)).has_value());
    BOOST_TEST(!v3d::audio::orthonormal(facing(origin, ahead, {0.0F, infinity, 0.0F})).has_value());

    v3d::audio::Listener sideless = facing(origin, ahead, up);
    sideless.right = glm::vec3(0.0F);
    BOOST_TEST(!v3d::audio::orthonormal(sideless).has_value());
    sideless.right = glm::vec3(0.0F, 2.0F, -1.0F);
    BOOST_TEST(!v3d::audio::orthonormal(sideless).has_value());
    sideless.right = glm::vec3(nan, 0.0F, 0.0F);
    BOOST_TEST(!v3d::audio::orthonormal(sideless).has_value());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
