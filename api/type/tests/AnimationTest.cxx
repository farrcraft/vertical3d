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
#include <cstddef>
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
 * The bending strip's skeleton, as api/asset/tests/data/make_skin_fixture.ts writes it: root,
 * middle and top in a chain a unit apart along +y, each bound where it stands.
 **/
Skeleton strip() {
    Skeleton skeleton;
    for (int joint = 0; joint < 3; joint++) {
        Skeleton::Joint read;
        read.parent = joint - 1;
        read.translation = glm::vec3(0.0F, joint == 0 ? 0.0F : 1.0F, 0.0F);
        read.inverseBind = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, -static_cast<float>(joint), 0.0F));
        skeleton.joints.push_back(read);
    }
    return skeleton;
}

/**
 * A quarter turn about +z, stored x, y, z, w as a clip stores a rotation.
 **/
glm::vec4 quarterTurn() {
    const float half = std::sqrt(0.5F);
    return glm::vec4(0.0F, 0.0F, half, half);
}

/**
 * The strip's middle joint turning a quarter about +z over one second.
 **/
Clip bend(Channel::Interpolation interpolation) {
    Channel turn;
    turn.joint = 1;
    turn.path = Channel::Path::Rotation;
    turn.interpolation = interpolation;
    turn.times = {0.0F, 1.0F};
    turn.values = {glm::vec4(0.0F, 0.0F, 0.0F, 1.0F), quarterTurn()};
    Clip clip;
    clip.name = "bend";
    clip.duration = 1.0F;
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
    BOOST_CHECK_SMALL(actual.x - expected.x, 1e-5F);
    BOOST_CHECK_SMALL(actual.y - expected.y, 1e-5F);
    BOOST_CHECK_SMALL(actual.z - expected.z, 1e-5F);
}

/**
 * The angle of a rotation about +z, which is all the bend turns.
 **/
float angleAboutZ(const glm::quat& rotation) {
    return 2.0F * std::atan2(rotation.z, rotation.w);
}

BOOST_AUTO_TEST_SUITE(animation_test)

/**
 * The palette at the rest pose is the identity for every joint, exactly, so a skin at rest
 * draws the same picture as its unskinned mesh.
 **/
BOOST_AUTO_TEST_CASE(animation_the_rest_palette_is_the_identity_test) {
    const Skeleton skeleton = strip();
    std::vector<glm::mat4> matrices;
    v3d::type::animation::palette(skeleton, v3d::type::animation::rest(skeleton), &matrices);

    BOOST_REQUIRE_EQUAL(matrices.size(), 3U);
    for (const glm::mat4& matrix : matrices) {
        BOOST_CHECK(matrix == glm::mat4(1.0F));
    }
}

/**
 * The skeleton's root stands above every root joint, so a root that moves the skeleton moves
 * every joint's global matrix - and the palette, which a pose bound under no root does not
 * expect.
 **/
BOOST_AUTO_TEST_CASE(animation_the_root_places_the_skeleton_test) {
    Skeleton skeleton = strip();
    skeleton.root = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 4.0F));

    checkNear(standing(skeleton, v3d::type::animation::rest(skeleton), 2), glm::vec3(0.0F, 2.0F, 4.0F));
}

/**
 * Sampling the bend at its keys and between them. At 0 it is the rest pose. At 1 it is a
 * quarter turn with the top joint swung to (-1, 1, 0). At a half it is an eighth turn with
 * nothing else moved.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_linear_rotation_test) {
    const Skeleton skeleton = strip();
    const Clip clip = bend(Channel::Interpolation::Linear);

    Pose pose = v3d::type::animation::rest(skeleton);
    v3d::type::animation::sample(clip, 0.0F, &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6F);
    checkNear(standing(skeleton, pose, 2), glm::vec3(0.0F, 2.0F, 0.0F));

    v3d::type::animation::sample(clip, 1.0F, &pose);
    BOOST_CHECK_CLOSE(angleAboutZ(pose.joints[1].rotation), glm::half_pi<float>(), 1e-4F);
    checkNear(standing(skeleton, pose, 2), glm::vec3(-1.0F, 1.0F, 0.0F));

    v3d::type::animation::sample(clip, 0.5F, &pose);
    BOOST_CHECK_CLOSE(angleAboutZ(pose.joints[1].rotation), glm::quarter_pi<float>(), 1e-4F);
    BOOST_CHECK(pose.joints[0].rotation == glm::identity<glm::quat>());
    BOOST_CHECK(pose.joints[2].rotation == glm::identity<glm::quat>());
    BOOST_CHECK(pose.joints[1].translation == glm::vec3(0.0F, 1.0F, 0.0F));
}

/**
 * A stepped channel holds each key until the next one.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_step_test) {
    const Skeleton skeleton = strip();
    const Clip clip = bend(Channel::Interpolation::Step);

    Pose pose = v3d::type::animation::rest(skeleton);
    v3d::type::animation::sample(clip, 0.99F, &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6F);
    v3d::type::animation::sample(clip, 1.0F, &pose);
    BOOST_CHECK_CLOSE(angleAboutZ(pose.joints[1].rotation), glm::half_pi<float>(), 1e-4F);
}

/**
 * A time that is not finite reads as the first key, rather than reading past the last.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_time_that_is_not_finite_test) {
    const Skeleton skeleton = strip();
    const Clip clip = bend(Channel::Interpolation::Linear);

    Pose pose = v3d::type::animation::rest(skeleton);
    v3d::type::animation::sample(clip, std::nanf(""), &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6F);
    v3d::type::animation::sample(clip, INFINITY, &pose);
    BOOST_CHECK_SMALL(angleAboutZ(pose.joints[1].rotation), 1e-6F);
}

/**
 * A cubic spline gives each key's value at the key, and between them the Hermite blend of the
 * values and tangents. The top joint rises from 1 to 2 with an out tangent of +x and an in
 * tangent of -x. At a half that puts it a quarter along x: 0.125 of each tangent, the second
 * negated by its basis and again by its sign.
 **/
BOOST_AUTO_TEST_CASE(animation_sampling_a_cubic_spline_test) {
    Channel rise;
    rise.joint = 2;
    rise.path = Channel::Path::Translation;
    rise.interpolation = Channel::Interpolation::CubicSpline;
    rise.times = {0.0F, 1.0F};
    rise.values = {
        glm::vec4(0.0F), glm::vec4(0.0F, 1.0F, 0.0F, 0.0F), glm::vec4(1.0F, 0.0F, 0.0F, 0.0F),
        glm::vec4(-1.0F, 0.0F, 0.0F, 0.0F), glm::vec4(0.0F, 2.0F, 0.0F, 0.0F), glm::vec4(0.0F),
    };
    Clip clip;
    clip.duration = 1.0F;
    clip.channels.push_back(rise);

    Pose pose = v3d::type::animation::rest(strip());
    v3d::type::animation::sample(clip, 0.0F, &pose);
    BOOST_CHECK(pose.joints[2].translation == glm::vec3(0.0F, 1.0F, 0.0F));
    v3d::type::animation::sample(clip, 1.0F, &pose);
    BOOST_CHECK(pose.joints[2].translation == glm::vec3(0.0F, 2.0F, 0.0F));
    v3d::type::animation::sample(clip, 0.5F, &pose);
    BOOST_CHECK(pose.joints[2].translation == glm::vec3(0.25F, 1.5F, 0.0F));
}

/**
 * Half of the bent pose and the rest pose is an eighth turn, and the ends of the weight give
 * either pose exactly.
 **/
BOOST_AUTO_TEST_CASE(animation_blending_two_poses_test) {
    const Skeleton skeleton = strip();
    const Pose rested = v3d::type::animation::rest(skeleton);
    Pose bent = rested;
    v3d::type::animation::sample(bend(Channel::Interpolation::Linear), 1.0F, &bent);

    const Pose half = v3d::type::animation::blend(rested, bent, 0.5F);
    BOOST_CHECK_CLOSE(angleAboutZ(half.joints[1].rotation), glm::quarter_pi<float>(), 1e-4F);

    BOOST_CHECK(v3d::type::animation::blend(rested, bent, 0.0F).joints[1].rotation == rested.joints[1].rotation);
    BOOST_CHECK(v3d::type::animation::blend(rested, bent, 1.0F).joints[1].rotation == bent.joints[1].rotation);
}

/**
 * A looping clock keeps its time unwrapped, samples it wrapped, and reports every wrap a step
 * passes.
 **/
BOOST_AUTO_TEST_CASE(animation_a_looping_clock_test) {
    const Clock clock(1.0F, true);

    const float later = clock.advance(0.75F, 0.5F);
    BOOST_CHECK_EQUAL(later, 1.25F);
    BOOST_CHECK_EQUAL(clock.sample(later), 0.25F);
    BOOST_CHECK_EQUAL(clock.crossed(0.75F, later, 1.0F), 1U);
    BOOST_CHECK_EQUAL(clock.crossed(0.75F, later, 0.0F), 1U);
    BOOST_CHECK(!clock.finished(later));

    // a step longer than the clip passes the end once a loop
    BOOST_CHECK_EQUAL(clock.crossed(0.5F, clock.advance(0.5F, 3.0F), 1.0F), 3U);
}

/**
 * A clamped clock stops at its duration, and reports the end once - the step that reaches it,
 * and not the steps it then stands there for.
 **/
BOOST_AUTO_TEST_CASE(animation_a_clamped_clock_test) {
    const Clock clock(1.0F, false);

    const float end = clock.advance(0.75F, 0.5F);
    BOOST_CHECK_EQUAL(end, 1.0F);
    BOOST_CHECK(clock.finished(end));
    BOOST_CHECK_EQUAL(clock.crossed(0.75F, end, 1.0F), 1U);
    BOOST_CHECK_EQUAL(clock.crossed(end, clock.advance(end, 0.5F), 1.0F), 0U);
}

/**
 * A marker is passed by the step that reaches it, and not by the one that starts on it, so it
 * is reported once however the steps fall.
 **/
BOOST_AUTO_TEST_CASE(animation_a_marker_is_crossed_once_test) {
    const Clock clock(1.0F, true);

    BOOST_CHECK_EQUAL(clock.crossed(0.25F, 0.5F, 0.5F), 1U);
    BOOST_CHECK_EQUAL(clock.crossed(0.5F, 0.75F, 0.5F), 0U);
    BOOST_CHECK_EQUAL(clock.crossed(0.25F, 0.4F, 0.5F), 0U);
    // and a looping one passes it again the loop after
    BOOST_CHECK_EQUAL(clock.crossed(1.25F, 1.5F, 0.5F), 1U);
}

/**
 * A step to or from a time that is not finite passes no marker, and a marker that is not a
 * number is never passed. A count too large to hold is held at its largest value.
 **/
BOOST_AUTO_TEST_CASE(animation_a_clock_step_that_is_not_finite_test) {
    const Clock looping(1.0F, true);
    BOOST_CHECK_EQUAL(looping.crossed(0.5F, INFINITY, 1.0F), 0U);
    BOOST_CHECK_EQUAL(looping.crossed(-INFINITY, 0.5F, 1.0F), 0U);
    BOOST_CHECK_EQUAL(looping.crossed(0.5F, std::nanf(""), 1.0F), 0U);
    BOOST_CHECK_EQUAL(looping.crossed(std::nanf(""), 0.5F, 1.0F), 0U);
    BOOST_CHECK_EQUAL(looping.crossed(0.25F, 0.75F, std::nanf("")), 0U);
    BOOST_CHECK_EQUAL(looping.crossed(0.0F, 1e30F, 1.0F), std::numeric_limits<uint32_t>::max());

    const Clock clamped(1.0F, false);
    BOOST_CHECK_EQUAL(clamped.crossed(0.5F, INFINITY, 1.0F), 0U);
    BOOST_CHECK_EQUAL(clamped.crossed(0.5F, std::nanf(""), 1.0F), 0U);
}

/**
 * A clip of no length never moves, rather than dividing by its duration.
 **/
BOOST_AUTO_TEST_CASE(animation_an_empty_clock_test) {
    const Clock clock(0.0F, true);

    BOOST_CHECK_EQUAL(clock.advance(0.0F, 0.5F), 0.0F);
    BOOST_CHECK_EQUAL(clock.sample(3.0F), 0.0F);
    BOOST_CHECK_EQUAL(clock.crossed(0.0F, 1.0F, 0.0F), 0U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
