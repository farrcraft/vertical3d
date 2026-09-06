/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Transform.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec4.hpp>

namespace {

bool near(const glm::vec3& a, const glm::vec3& b) {
    const float tolerance = 0.0001f;
    return glm::abs(a.x - b.x) < tolerance && glm::abs(a.y - b.y) < tolerance && glm::abs(a.z - b.z) < tolerance;
}

glm::vec3 apply(const glm::mat4& transform, const glm::vec3& point) {
    glm::vec4 result = transform * glm::vec4(point, 1.0f);
    return glm::vec3(result);
}

};  // namespace

BOOST_AUTO_TEST_CASE(transform_identity_test) {
    v3d::dag::Transform transform;

    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(0.0f, 0.0f, 0.0f)), true);
    BOOST_CHECK_EQUAL(near(transform.scale(), glm::vec3(1.0f, 1.0f, 1.0f)), true);
    // a default constructed transform has to leave a point where it found it, which the
    // uninitialised quaternion of a default glm::quat does not
    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(3.0f, 4.0f, 5.0f)), glm::vec3(3.0f, 4.0f, 5.0f)), true);
}

BOOST_AUTO_TEST_CASE(transform_translation_test) {
    v3d::dag::Transform transform;

    // the setter sets, where translate() accumulates - a manipulator drag measures a delta,
    // and a loaded scene names a position
    transform.translation(glm::vec3(1.0f, 2.0f, 3.0f));
    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(1.0f, 2.0f, 3.0f)), true);

    transform.translation(glm::vec3(4.0f, 5.0f, 6.0f));
    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(4.0f, 5.0f, 6.0f)), true);

    transform.translate(glm::vec3(1.0f, 1.0f, 1.0f));
    BOOST_CHECK_EQUAL(near(transform.translation(), glm::vec3(5.0f, 6.0f, 7.0f)), true);

    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(0.0f, 0.0f, 0.0f)), glm::vec3(5.0f, 6.0f, 7.0f)), true);
}

BOOST_AUTO_TEST_CASE(transform_compose_test) {
    v3d::dag::Transform transform;

    // a quarter turn about z, then a non-uniform scale: scale is applied first, so it acts
    // along the object's own axes rather than the rotated ones
    transform.rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)));
    transform.scale(glm::vec3(2.0f, 3.0f, 1.0f));
    transform.translation(glm::vec3(10.0f, 0.0f, 0.0f));

    // x scales by 2 and then turns onto +y
    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(10.0f, 2.0f, 0.0f)), true);
    // y scales by 3 and then turns onto -x
    BOOST_CHECK_EQUAL(near(apply(transform.matrix(), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(7.0f, 0.0f, 0.0f)), true);
}
