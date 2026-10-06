/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/Skeleton.h>
#include <api/type/animation/Channel.h>
#include <api/type/animation/Clip.h>
#include <api/type/animation/Clock.h>
#include <api/type/animation/Pose.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using v3d::type::Skeleton;
using v3d::type::animation::Channel;
using v3d::type::animation::Clip;
using v3d::type::animation::Clock;
using v3d::type::animation::Pose;

namespace {

/**
 * The bending strip's skeleton, as api/asset/tests/data/make_skin_fixture.py writes it: root,
 * middle and top in a chain a unit apart along +y, each bound where it stands.
 **/
Skeleton strip() {
    Skeleton skeleton;
    for (int joint = 0; joint < 3; joint++) {
        Skeleton::Joint read;
        read.parent = joint - 1;
        read.translation = glm::vec3(0.0f, joint == 0 ? 0.0f : 1.0f, 0.0f);
        read.inverseBind = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -static_cast<float>(joint), 0.0f));
        skeleton.joints.push_back(read);
    }
    return skeleton;
}

/**
 * A quarter turn about +z, stored x, y, z, w as a clip stores a rotation.
 **/
glm::vec4 quarterTurn() {
    const float half = std::sqrt(0.5f);
    return glm::vec4(0.0f, 0.0f, half, half);
}

/**
 * The strip's middle joint turning a quarter about +z over one second.
 **/
Clip bend(Channel::Interpolation interpolation) {
    Channel turn;
    turn.joint = 1;
    turn.path = Channel::Path::Rotation;
    turn.interpolation = interpolation;
    turn.times = {0.0f, 1.0f};
    turn.values = {glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), quarterTurn()};
    Clip clip;
    clip.name = "bend";
    clip.duration = 1.0f;
    clip.channels.push_back(turn);
    return clip;
}

/**
 * Where a joint stands in the world under a pose: the translation of its global matrix, which
 * is its skinning matrix undone by its inverse bind.
 **/
glm::vec3 standing(const Skeleton& skeleton, const Pose& pose, std::size_t joint) {
    std::vector<glm::mat4> matrices;
    v3d::type::animation::palette(skeleton, pose, &matrices);
    return glm::vec3((matrices[joint] * glm::inverse(skeleton.joints[joint].inverseBind))[3]);
}

void checkNear(const glm::vec3& actual, const glm::vec3& expected) {
    BOOST_CHECK_SMALL(actual.x - expected.x, 1e-5f);
    BOOST_CHECK_SMALL(actual.y - expected.y, 1e-5f);
    BOOST_CHECK_SMALL(actual.z - expected.z, 1e-5f);
}

/**
 * The angle of a rotation about +z, which is all the bend turns.
 **/
float angleAboutZ(const glm::quat& rotation) {
    return 2.0f * std::atan2(rotation.z, rotation.w);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(animation_test)

/**
 * The palette at the rest pose is the identity for every joint, exactly, so a skin at rest
 * draws the same picture as its unskinned mesh.
 **/
BOOST_AUTO_TEST_CASE(animation_the_rest_palette_is_the_identity_test) {
    const Skeleton skeleton = strip();
    std::vector<glm::mat4> matrices;
    v3d::type::animation::palette(skeleton, v3d::type::animation::rest(skeleton), &matrices);

    BOOST_REQUIRE_EQUAL(matrices.size(), 3u);
    for (const glm::mat4& matrix : matrices) {
        BOOST_CHECK(matrix == glm::mat4(1.0f));
    }
}

/**
 * The skeleton's root stands above every root joint, so a root that moves the skeleton moves
 * every joint's global matrix - and the palette, which a pose bound under no root does not
 * expect.
 **/
BOOST_AUTO_TEST_CASE(animation_the_root_places_the_skeleton_test) {
    Skeleton skeleton = strip();
    skeleton.root = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 4.0f));

    checkNear(standing(skeleton, v3d::type::animation::rest(skeleton), 2), glm::vec3(0.0f, 2.0f, 4.0f));
}

/**
 * Sampling the bend at its keys and between them: the rest pose at 0, a quarter turn at 1 with
 * the top joint swung to (-1, 1, 0), and an eighth turn at a half with nothing else moved.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_linear_rotation_test) {
    const Skeleton skeleton = strip();
    const Clip clip = bend(Channel::Interpolation::Linear);

    Pose pose = v3d::type::animation::rest(skeleton);
    v3d::type::animation::sample(clip, 0.0f, &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6f);
    checkNear(standing(skeleton, pose, 2), glm::vec3(0.0f, 2.0f, 0.0f));

    v3d::type::animation::sample(clip, 1.0f, &pose);
    BOOST_CHECK_CLOSE(angleAboutZ(pose.joints[1].rotation), glm::half_pi<float>(), 1e-4f);
    checkNear(standing(skeleton, pose, 2), glm::vec3(-1.0f, 1.0f, 0.0f));

    v3d::type::animation::sample(clip, 0.5f, &pose);
    BOOST_CHECK_CLOSE(angleAboutZ(pose.joints[1].rotation), glm::quarter_pi<float>(), 1e-4f);
    BOOST_CHECK(pose.joints[0].rotation == glm::identity<glm::quat>());
    BOOST_CHECK(pose.joints[2].rotation == glm::identity<glm::quat>());
    BOOST_CHECK(pose.joints[1].translation == glm::vec3(0.0f, 1.0f, 0.0f));
}

/**
 * A stepped channel holds each key until the next one.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_step_test) {
    const Skeleton skeleton = strip();
    const Clip clip = bend(Channel::Interpolation::Step);

    Pose pose = v3d::type::animation::rest(skeleton);
    v3d::type::animation::sample(clip, 0.99f, &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6f);
    v3d::type::animation::sample(clip, 1.0f, &pose);
    BOOST_CHECK_CLOSE(angleAboutZ(pose.joints[1].rotation), glm::half_pi<float>(), 1e-4f);
}

/**
 * A time that is not finite reads as the first key, rather than reading past the last.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_time_that_is_not_finite_test) {
    const Skeleton skeleton = strip();
    const Clip clip = bend(Channel::Interpolation::Linear);

    Pose pose = v3d::type::animation::rest(skeleton);
    v3d::type::animation::sample(clip, std::nanf(""), &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6f);
    v3d::type::animation::sample(clip, INFINITY, &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6f);
}

/**
 * A cubic spline gives each key's value at the key, and between them the Hermite blend of the
 * values and tangents. The top joint rises from 1 to 2 with an out tangent of +x and an in
 * tangent of -x, which at a half puts it a quarter along x: 0.125 of each tangent, the second
 * negated by its basis and again by its sign.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_cubic_spline_test) {
    Channel rise;
    rise.joint = 2;
    rise.path = Channel::Path::Translation;
    rise.interpolation = Channel::Interpolation::CubicSpline;
    rise.times = {0.0f, 1.0f};
    rise.values = {
        glm::vec4(0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 2.0f, 0.0f, 0.0f), glm::vec4(0.0f),
    };
    Clip clip;
    clip.duration = 1.0f;
    clip.channels.push_back(rise);

    Pose pose = v3d::type::animation::rest(strip());
    v3d::type::animation::sample(clip, 0.0f, &pose);
    BOOST_CHECK(pose.joints[2].translation == glm::vec3(0.0f, 1.0f, 0.0f));
    v3d::type::animation::sample(clip, 1.0f, &pose);
    BOOST_CHECK(pose.joints[2].translation == glm::vec3(0.0f, 2.0f, 0.0f));
    v3d::type::animation::sample(clip, 0.5f, &pose);
    BOOST_CHECK(pose.joints[2].translation == glm::vec3(0.25f, 1.5f, 0.0f));
}

/**
 * Half of the bent pose and the rest pose is an eighth turn, and the ends of the weight give
 * either pose exactly.
 **/
BOOST_AUTO_TEST_CASE(animation_blending_two_poses_test) {
    const Skeleton skeleton = strip();
    const Pose rested = v3d::type::animation::rest(skeleton);
    Pose bent = rested;
    v3d::type::animation::sample(bend(Channel::Interpolation::Linear), 1.0f, &bent);

    const Pose half = v3d::type::animation::blend(rested, bent, 0.5f);
    BOOST_CHECK_CLOSE(angleAboutZ(half.joints[1].rotation), glm::quarter_pi<float>(), 1e-4f);

    BOOST_CHECK(v3d::type::animation::blend(rested, bent, 0.0f).joints[1].rotation == rested.joints[1].rotation);
    BOOST_CHECK(v3d::type::animation::blend(rested, bent, 1.0f).joints[1].rotation == bent.joints[1].rotation);
}

/**
 * A looping clock keeps its time unwrapped, samples it wrapped, and reports every wrap a step
 * passes.
 **/
BOOST_AUTO_TEST_CASE(animation_a_looping_clock_test) {
    const Clock clock(1.0f, true);

    const float later = clock.advance(0.75f, 0.5f);
    BOOST_CHECK_EQUAL(later, 1.25f);
    BOOST_CHECK_EQUAL(clock.sample(later), 0.25f);
    BOOST_CHECK_EQUAL(clock.crossed(0.75f, later, 1.0f), 1u);
    BOOST_CHECK_EQUAL(clock.crossed(0.75f, later, 0.0f), 1u);
    BOOST_CHECK(!clock.finished(later));

    // a step longer than the clip passes the end once a loop
    BOOST_CHECK_EQUAL(clock.crossed(0.5f, clock.advance(0.5f, 3.0f), 1.0f), 3u);
}

/**
 * A clamped clock stops at its duration, and reports the end once - the step that reaches it,
 * and not the steps it then stands there for.
 **/
BOOST_AUTO_TEST_CASE(animation_a_clamped_clock_test) {
    const Clock clock(1.0f, false);

    const float end = clock.advance(0.75f, 0.5f);
    BOOST_CHECK_EQUAL(end, 1.0f);
    BOOST_CHECK(clock.finished(end));
    BOOST_CHECK_EQUAL(clock.crossed(0.75f, end, 1.0f), 1u);
    BOOST_CHECK_EQUAL(clock.crossed(end, clock.advance(end, 0.5f), 1.0f), 0u);
}

/**
 * A marker is passed by the step that reaches it, and not by the one that starts on it, so it
 * is reported once however the steps fall.
 **/
BOOST_AUTO_TEST_CASE(animation_a_marker_is_crossed_once_test) {
    const Clock clock(1.0f, true);

    BOOST_CHECK_EQUAL(clock.crossed(0.25f, 0.5f, 0.5f), 1u);
    BOOST_CHECK_EQUAL(clock.crossed(0.5f, 0.75f, 0.5f), 0u);
    BOOST_CHECK_EQUAL(clock.crossed(0.25f, 0.4f, 0.5f), 0u);
    // and a looping one passes it again the loop after
    BOOST_CHECK_EQUAL(clock.crossed(1.25f, 1.5f, 0.5f), 1u);
}

/**
 * A step to or from a time that is not finite passes no marker, and a marker that is not a
 * number is never passed. A count too large to hold is held at its largest value.
 **/
BOOST_AUTO_TEST_CASE(animation_a_clock_step_that_is_not_finite_test) {
    const Clock looping(1.0f, true);
    BOOST_CHECK_EQUAL(looping.crossed(0.5f, INFINITY, 1.0f), 0u);
    BOOST_CHECK_EQUAL(looping.crossed(-INFINITY, 0.5f, 1.0f), 0u);
    BOOST_CHECK_EQUAL(looping.crossed(0.5f, std::nanf(""), 1.0f), 0u);
    BOOST_CHECK_EQUAL(looping.crossed(std::nanf(""), 0.5f, 1.0f), 0u);
    BOOST_CHECK_EQUAL(looping.crossed(0.25f, 0.75f, std::nanf("")), 0u);
    BOOST_CHECK_EQUAL(looping.crossed(0.0f, 1e30f, 1.0f), std::numeric_limits<uint32_t>::max());

    const Clock clamped(1.0f, false);
    BOOST_CHECK_EQUAL(clamped.crossed(0.5f, INFINITY, 1.0f), 0u);
    BOOST_CHECK_EQUAL(clamped.crossed(0.5f, std::nanf(""), 1.0f), 0u);
}

/**
 * A clip of no length never moves, rather than dividing by its duration.
 **/
BOOST_AUTO_TEST_CASE(animation_an_empty_clock_test) {
    const Clock clock(0.0f, true);

    BOOST_CHECK_EQUAL(clock.advance(0.0f, 0.5f), 0.0f);
    BOOST_CHECK_EQUAL(clock.sample(3.0f), 0.0f);
    BOOST_CHECK_EQUAL(clock.crossed(0.0f, 1.0f, 0.0f), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
