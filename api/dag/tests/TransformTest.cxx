/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/dag/Transform.h>
#include <api/type/Transform.h>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

bool near(const glm::vec3& actual, const glm::vec3& expected) {
    const float tolerance = 0.0001F;
    return glm::abs(actual.x - expected.x) < tolerance && glm::abs(actual.y - expected.y) < tolerance && glm::abs(actual.z - expected.z) < tolerance;
}

glm::vec3 apply(const glm::mat4& transform, const glm::vec3& point) {
    const glm::vec4 result = transform * glm::vec4(point, 1.0F);
    return glm::vec3(result);
}

BOOST_AUTO_TEST_CASE(transform_identity_test) {
    const v3d::dag::Transform transform;

    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(0.0F, 0.0F, 0.0F)), true);
    BOOST_CHECK_EQUAL(near(transform.scale(), glm::vec3(1.0F, 1.0F, 1.0F)), true);
    // a default-constructed transform leaves a point where it is. A default glm::quat is
    // uninitialised, so this checks that the rotation starts as the identity
    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(3.0F, 4.0F, 5.0F)), glm::vec3(3.0F, 4.0F, 5.0F)), true);
}

BOOST_AUTO_TEST_CASE(transform_translation_test) {
    v3d::dag::Transform transform;

    // translation() replaces the position, and translate() adds to it
    transform.translation(glm::vec3(1.0F, 2.0F, 3.0F));
    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(1.0F, 2.0F, 3.0F)), true);

    transform.translation(glm::vec3(4.0F, 5.0F, 6.0F));
    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(4.0F, 5.0F, 6.0F)), true);

    transform.translate(glm::vec3(1.0F, 1.0F, 1.0F));
    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(5.0F, 6.0F, 7.0F)), true);

    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(0.0F, 0.0F, 0.0F)), glm::vec3(5.0F, 6.0F, 7.0F)), true);
}

BOOST_AUTO_TEST_CASE(transform_compose_test) {
    v3d::dag::Transform transform;

    // a quarter turn about z and a non-uniform scale. The scale is applied first, so it acts
    // along the object's own axes, not the rotated ones
    transform.rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F)));
    transform.scale(glm::vec3(2.0F, 3.0F, 1.0F));
    transform.translation(glm::vec3(10.0F, 0.0F, 0.0F));

    // x scales by 2 and then turns onto +y
    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(1.0F, 0.0F, 0.0F)), glm::vec3(10.0F, 2.0F, 0.0F)), true);
    // y scales by 3 and then turns onto -x
    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(0.0F, 1.0F, 0.0F)), glm::vec3(7.0F, 0.0F, 0.0F)), true);
}

/**
 * value() holds what the setters and translate() wrote, and its matrix() equals the
 * Transform's matrix().
 **/
BOOST_AUTO_TEST_CASE(transform_value_test) {
    v3d::dag::Transform transform;
    transform.translation(glm::vec3(1.0F, 2.0F, 3.0F));
    transform.scale(glm::vec3(2.0F));
    transform.translate(glm::vec3(1.0F, 0.0F, 0.0F));

    const v3d::type::Transform & value = transform.value();
    BOOST_CHECK_EQUAL(value.position.x, 2.0F);
    BOOST_CHECK_EQUAL(value.scale.y, 2.0F);
    BOOST_CHECK_EQUAL((value.matrix() == transform.matrix()), true);
}

BOOST_AUTO_TEST_CASE(transform_changes_test) {
    v3d::dag::Transform transform;
    BOOST_CHECK_EQUAL(transform.changes(), 0U);

    // every setter counts, a setter that sets what is already there included
    transform.translation(glm::vec3(1.0F, 0.0F, 0.0F));
    transform.translate(glm::vec3(0.0F));
    transform.rotation(glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    transform.scale(glm::vec3(2.0F));
    BOOST_CHECK_EQUAL(transform.changes(), 4U);

    // and reading does not
    static_cast<void>(transform.matrix());
    BOOST_CHECK_EQUAL(transform.changes(), 4U);
}

};  // namespace
