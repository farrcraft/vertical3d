/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Properties.h>

#include <cmath>
#include <cstdint>
#include <map>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * Whether two rotations turn things the same way. A quaternion and its negation do.
 **/
bool sameTurn(const glm::quat& first, const glm::quat& second) {
    return std::abs(glm::dot(glm::normalize(first), glm::normalize(second))) > 0.9999F;
}

BOOST_AUTO_TEST_CASE(properties_declared_test) {
    // every kind has its placement, and a camera and a light add what they hold
    BOOST_CHECK_EQUAL(v3d::editor::properties(v3d::editor::Item::Kind::Mesh).size(), 9U);
    BOOST_CHECK_EQUAL(v3d::editor::properties(v3d::editor::Item::Kind::Group).size(), 9U);
    BOOST_CHECK_EQUAL(v3d::editor::properties(v3d::editor::Item::Kind::Camera).size(), 13U);
    BOOST_CHECK_EQUAL(v3d::editor::properties(v3d::editor::Item::Kind::Light).size(), 15U);
    BOOST_CHECK(v3d::editor::declares(v3d::editor::Item::Kind::Mesh, "rotation.y"));
    BOOST_CHECK(v3d::editor::declares(v3d::editor::Item::Kind::Camera, "lens.fov"));
    BOOST_CHECK(!v3d::editor::declares(v3d::editor::Item::Kind::Mesh, "lens.fov"));
    BOOST_CHECK(!v3d::editor::declares(v3d::editor::Item::Kind::Light, "rotation"));
}

BOOST_AUTO_TEST_CASE(properties_values_test) {
    v3d::editor::Light light;
    light.translation(glm::vec3(1.0F, 2.0F, 3.0F));
    light.scale(glm::vec3(4.0F));
    light.emission({ .colour = glm::vec3(0.25F, 0.5F, 0.75F), .intensity = 9.0F });

    const std::map<std::string, float> found = v3d::editor::values(light, glm::vec3(0.0F));
    BOOST_CHECK_EQUAL(found.size(), 15U);
    BOOST_CHECK_EQUAL(found.at("translation.y"), 2.0F);
    BOOST_CHECK_EQUAL(found.at("scale.z"), 4.0F);
    BOOST_CHECK_EQUAL(found.at("emission.intensity"), 9.0F);
    BOOST_CHECK_EQUAL(found.at("emission.green"), 0.5F);
    BOOST_CHECK_SMALL(found.at("rotation.x"), 0.0001F);

    v3d::editor::Camera camera;
    camera.lens({ .fov = 35.0F, .extent = 2.0F, .clipping = glm::vec2(0.5F, 80.0F) });
    const std::map<std::string, float> lens = v3d::editor::values(camera, glm::vec3(0.0F));
    BOOST_CHECK_EQUAL(lens.at("lens.fov"), 35.0F);
    BOOST_CHECK_EQUAL(lens.at("lens.near"), 0.5F);
    BOOST_CHECK_EQUAL(lens.at("lens.far"), 80.0F);
}

BOOST_AUTO_TEST_CASE(properties_pose_test) {
    v3d::editor::Light light;
    light.translation(glm::vec3(1.0F, 2.0F, 3.0F));
    const glm::quat turned = v3d::editor::fromEuler(glm::vec3(10.0F, 20.0F, 30.0F));
    light.rotation(turned);
    light.emission({ .intensity = 5.0F });
    const std::uint64_t before = light.revision();
    const v3d::editor::Pose held = v3d::editor::pose(light);
    BOOST_CHECK_EQUAL(held.emission.intensity, 5.0F);

    // only the parts a value names are set, and each keeps whatever the values leave out. A
    // path the kind does not declare does nothing, and the item is not changed
    const v3d::editor::Pose posed = v3d::editor::posed(v3d::editor::Item::Kind::Light, held,
        { { "translation.x", 7.0F }, { "emission.intensity", 2.0F }, { "lens.fov", 3.0F } });
    BOOST_CHECK(posed.placement.position == glm::vec3(7.0F, 2.0F, 3.0F));
    BOOST_CHECK(posed.placement.rotation == turned);
    BOOST_CHECK_EQUAL(posed.emission.intensity, 2.0F);
    BOOST_CHECK_EQUAL(posed.lens.fov, held.lens.fov);
    BOOST_CHECK_EQUAL(light.revision(), before);
    BOOST_CHECK_EQUAL(light.emission().intensity, 5.0F);

    // a rotation is set whole, from the angles named and those closest to zero for the rest
    const v3d::editor::Pose turning = v3d::editor::posed(v3d::editor::Item::Kind::Light, held, { { "rotation.z", 90.0F } });
    BOOST_CHECK(sameTurn(turning.placement.rotation, v3d::editor::fromEuler(glm::vec3(10.0F, 20.0F, 90.0F))));
}

BOOST_AUTO_TEST_CASE(properties_euler_test) {
    // x turns first, then y, then z
    const glm::quat order = v3d::editor::fromEuler(glm::vec3(90.0F, 0.0F, 90.0F));
    BOOST_CHECK_SMALL(glm::distance(order * glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(1.0F, 0.0F, 0.0F)), 0.0001F);

    // the angles come back as they went in, even past a quarter turn about y, where the other
    // set of angles that gives the same rotation is the principal one
    const glm::vec3 angles(30.0F, 200.0F, -45.0F);
    const glm::vec3 back = v3d::editor::eulerNear(v3d::editor::fromEuler(angles), angles);
    BOOST_CHECK_SMALL(glm::distance(back, angles), 0.01F);

    // whole turns are added to come closest to the angles given
    const glm::quat nearly = v3d::editor::fromEuler(glm::vec3(0.0F, 0.0F, 350.0F));
    BOOST_CHECK_SMALL(v3d::editor::eulerNear(nearly, glm::vec3(0.0F, 0.0F, -20.0F)).z + 10.0F, 0.01F);
    BOOST_CHECK_SMALL(v3d::editor::eulerNear(nearly, glm::vec3(0.0F, 0.0F, 700.0F)).z - 710.0F, 0.01F);

    // at a quarter turn about y the angles still give the rotation back
    const glm::quat locked = v3d::editor::fromEuler(glm::vec3(10.0F, 90.0F, 0.0F));
    BOOST_CHECK(sameTurn(v3d::editor::fromEuler(v3d::editor::eulerNear(locked, glm::vec3(0.0F))), locked));
}

};  // namespace
