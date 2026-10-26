/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/type/Transform.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>

#include <cstdint>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

bool near(const glm::vec3& actual, const glm::vec3& expected) {
    const float tolerance = 0.0001F;
    return glm::all(glm::lessThan(glm::abs(actual - expected), glm::vec3(tolerance)));
}

glm::vec3 apply(const glm::mat4& transform, const glm::vec3& point) {
    return glm::vec3(transform * glm::vec4(point, 1.0F));
}

BOOST_AUTO_TEST_CASE(item_identity_test) {
    const v3d::editor::Mesh mesh(boost::make_shared<v3d::brep::BRep>());
    const v3d::editor::Camera camera;
    const v3d::editor::Light light;

    // every kind has an id from one counter, so no two items share one
    BOOST_CHECK(mesh.id() != camera.id());
    BOOST_CHECK(camera.id() != light.id());
    BOOST_CHECK(mesh.id() != light.id());

    // a copy would share that id with its original, so an item cannot be copied
    static_assert(!std::is_copy_constructible_v<v3d::editor::Mesh>);
    static_assert(!std::is_copy_assignable_v<v3d::editor::Camera>);
}

BOOST_AUTO_TEST_CASE(item_kind_test) {
    const boost::shared_ptr<v3d::editor::Item> mesh =
        boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
    const boost::shared_ptr<v3d::editor::Item> camera = boost::make_shared<v3d::editor::Camera>();
    const boost::shared_ptr<v3d::editor::Item> light = boost::make_shared<v3d::editor::Light>();

    BOOST_CHECK(mesh->kind() == v3d::editor::Item::Kind::Mesh);
    BOOST_CHECK(camera->kind() == v3d::editor::Item::Kind::Camera);
    BOOST_CHECK(light->kind() == v3d::editor::Item::Kind::Light);

    // as() casts only to the kind the item is
    BOOST_CHECK(v3d::editor::as<v3d::editor::Mesh>(mesh).get() == mesh.get());
    BOOST_CHECK(!v3d::editor::as<v3d::editor::Camera>(mesh));
    BOOST_CHECK(v3d::editor::as<v3d::editor::Camera>(camera).get() == camera.get());
    BOOST_CHECK(!v3d::editor::as<v3d::editor::Light>(camera));
    BOOST_CHECK(v3d::editor::as<v3d::editor::Light>(light).get() == light.get());
    BOOST_CHECK(!v3d::editor::as<v3d::editor::Mesh>(light));
    BOOST_CHECK(!v3d::editor::as<v3d::editor::Mesh>(boost::shared_ptr<v3d::editor::Item>()));
}

BOOST_AUTO_TEST_CASE(item_placement_test) {
    v3d::editor::Light light;

    // the item's transform places it
    light.translation(glm::vec3(4.0F, 0.0F, 0.0F));
    const glm::vec4 placed = light.matrix() * glm::vec4(1.0F, 0.0F, 0.0F, 1.0F);
    BOOST_CHECK_CLOSE(placed.x, 5.0F, 0.01F);
}

/**
 * A later item has a larger id than any earlier one.
 **/
BOOST_AUTO_TEST_CASE(item_id_order_test) {
    const v3d::editor::Group first;
    const v3d::editor::Group second;
    BOOST_CHECK_GT(second.id(), first.id());
}

/**
 * A new item is at the origin, unrotated and unscaled. A default glm::quat is uninitialised, so
 * this checks that the rotation starts as the identity.
 **/
BOOST_AUTO_TEST_CASE(item_default_placement_test) {
    const v3d::editor::Group group;
    BOOST_CHECK(near(group.translation(), glm::vec3(0.0F)));
    BOOST_CHECK(near(group.scale(), glm::vec3(1.0F)));
    BOOST_CHECK(near(apply(group.matrix(), glm::vec3(3.0F, 4.0F, 5.0F)), glm::vec3(3.0F, 4.0F, 5.0F)));
}

/**
 * translation() replaces the position, and translate() adds to it.
 **/
BOOST_AUTO_TEST_CASE(item_translate_test) {
    v3d::editor::Group group;
    group.translation(glm::vec3(4.0F, 5.0F, 6.0F));
    group.translate(glm::vec3(1.0F, 1.0F, 1.0F));
    BOOST_CHECK(near(group.translation(), glm::vec3(5.0F, 6.0F, 7.0F)));
    BOOST_CHECK(near(apply(group.matrix(), glm::vec3(0.0F)), glm::vec3(5.0F, 6.0F, 7.0F)));
}

/**
 * The scale is applied first, so it acts along the item's own axes, not the rotated ones.
 **/
BOOST_AUTO_TEST_CASE(item_compose_test) {
    v3d::editor::Group group;
    group.rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F)));
    group.scale(glm::vec3(2.0F, 3.0F, 1.0F));
    group.translation(glm::vec3(10.0F, 0.0F, 0.0F));

    // x scales by 2 and then turns onto +y, and y scales by 3 and then turns onto -x
    BOOST_CHECK(near(apply(group.matrix(), glm::vec3(1.0F, 0.0F, 0.0F)), glm::vec3(10.0F, 2.0F, 0.0F)));
    BOOST_CHECK(near(apply(group.matrix(), glm::vec3(0.0F, 1.0F, 0.0F)), glm::vec3(7.0F, 0.0F, 0.0F)));
}

/**
 * placement() holds what the setters wrote, and setting it back restores all three parts.
 **/
BOOST_AUTO_TEST_CASE(item_whole_placement_test) {
    v3d::editor::Group group;
    group.translation(glm::vec3(1.0F, 2.0F, 3.0F));
    group.scale(glm::vec3(2.0F));
    const v3d::type::Transform kept = group.placement();
    BOOST_CHECK_EQUAL(kept.position.x, 1.0F);
    BOOST_CHECK((kept.matrix() == group.matrix()));

    group.translation(glm::vec3(0.0F));
    group.scale(glm::vec3(1.0F));
    group.placement(kept);
    BOOST_CHECK(near(group.translation(), glm::vec3(1.0F, 2.0F, 3.0F)));
    BOOST_CHECK(near(group.scale(), glm::vec3(2.0F)));
}

/**
 * Every placement setter moves the revision, one that writes what is already there included,
 * and reading does not.
 **/
BOOST_AUTO_TEST_CASE(item_placement_revision_test) {
    v3d::editor::Group group;
    const std::uint64_t start = group.revision();
    group.translation(glm::vec3(0.0F));
    group.translate(glm::vec3(0.0F));
    group.rotation(glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    group.scale(glm::vec3(1.0F));
    group.placement(group.placement());
    BOOST_CHECK_EQUAL(group.revision(), start + 5U);

    static_cast<void>(group.matrix());
    BOOST_CHECK_EQUAL(group.revision(), start + 5U);
}

BOOST_AUTO_TEST_CASE(item_selection_test) {
    const v3d::editor::Mesh mesh(boost::make_shared<v3d::brep::BRep>());
    v3d::editor::Camera camera;

    BOOST_CHECK_EQUAL(mesh.selected(), false);
    BOOST_CHECK_EQUAL(camera.selected(), false);
    camera.selected(true);
    BOOST_CHECK_EQUAL(camera.selected(), true);
    BOOST_CHECK_EQUAL(mesh.selected(), false);
}

BOOST_AUTO_TEST_CASE(mesh_geometry_test) {
    // an empty pointer becomes an empty mesh, so the geometry is always there to read
    const v3d::editor::Mesh empty((boost::shared_ptr<v3d::brep::BRep>()));
    BOOST_REQUIRE(empty.geometry());
    BOOST_CHECK_EQUAL(empty.geometry()->vertexCount(), 0U);

    // two items may show one mesh, each placed by its own transform
    const boost::shared_ptr<v3d::brep::BRep> shared = boost::make_shared<v3d::brep::BRep>();
    v3d::editor::Mesh first(shared);
    const v3d::editor::Mesh second(shared);
    BOOST_CHECK(first.geometry() == second.geometry());
    first.translation(glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_CHECK(first.translation() != second.translation());
}

BOOST_AUTO_TEST_CASE(camera_and_light_defaults_test) {
    const v3d::editor::Camera camera;
    BOOST_CHECK(camera.lens().projection == v3d::editor::Camera::Projection::Perspective);
    BOOST_CHECK_GT(camera.lens().clipping.y, camera.lens().clipping.x);

    // the RI shaders' own defaults: white, at one, and a spot cone of 30 degrees with a
    // 5 degree soft edge
    const v3d::editor::Light light;
    BOOST_CHECK(light.emission().type == v3d::editor::Light::Type::Point);
    BOOST_CHECK_CLOSE(light.emission().intensity, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(light.emission().cone, 0.523599F, 0.01F);
    BOOST_CHECK_CLOSE(light.emission().delta, 0.0872665F, 0.01F);
}

BOOST_AUTO_TEST_CASE(item_revision_test) {
    v3d::editor::Camera camera;
    v3d::editor::Light light;
    v3d::editor::Mesh mesh(boost::make_shared<v3d::brep::BRep>());

    // placement, what a kind holds, and a mesh's geometry each move the revision
    std::uint64_t before = camera.revision();
    camera.translation(glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_CHECK_GT(camera.revision(), before);
    before = camera.revision();
    camera.lens(v3d::editor::Camera::Lens());
    BOOST_CHECK_GT(camera.revision(), before);

    before = light.revision();
    light.emission(v3d::editor::Light::Emission());
    BOOST_CHECK_GT(light.revision(), before);

    before = mesh.revision();
    mesh.geometry()->touch();
    BOOST_CHECK_GT(mesh.revision(), before);

    // and a selection does not
    before = mesh.revision();
    mesh.selected(true);
    BOOST_CHECK_EQUAL(mesh.revision(), before);
}

};  // namespace
