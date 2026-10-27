/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/Light.h>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A view looking from one corner of the scene at another, as a camera profile.
 **/
v3d::type::camera::Profile corner(v3d::type::camera::Profile::Hand hand) {
    v3d::type::camera::Profile view("corner");
    view.hand(hand);
    view.orthographic(false);
    view.eye(glm::vec3(4.0F, 3.0F, -6.0F));
    view.lookat(glm::vec3(-1.0F, 0.5F, 2.0F));
    view.fov(50.0F);
    view.orthoZoom(7.0F);
    view.clipping(glm::vec2(0.25F, 400.0F));
    view.size(glm::uvec2(640, 480));
    return view;
}

/**
 * The largest difference between the elements of two matrices.
 **/
float apart(const glm::mat4& left, const glm::mat4& right) {
    float most = 0.0F;
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            most = glm::max(most, glm::abs(left[column][row] - right[column][row]));
        }
    }
    return most;
}

BOOST_AUTO_TEST_CASE(create_camera_at_the_view_test) {
    const v3d::type::camera::Profile view = corner(v3d::type::camera::Profile::Hand::UpCrossDirection);
    const boost::shared_ptr<v3d::editor::Camera> camera =
        v3d::editor::create_camera(view, v3d::editor::Camera::Projection::Perspective);

    // at the view's eye, its +z where the view looks and its +y the view's up
    BOOST_CHECK_SMALL(glm::distance(camera->translation(), view.eye()), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(camera->rotation() * glm::vec3(0.0F, 0.0F, 1.0F), glm::normalize(view.direction())), 0.001F);
    BOOST_CHECK_SMALL(glm::distance(camera->rotation() * glm::vec3(0.0F, 1.0F, 0.0F), glm::normalize(view.up())), 0.001F);

    // with the view's lens
    BOOST_CHECK_EQUAL(camera->lens().fov, 50.0F);
    BOOST_CHECK_EQUAL(camera->lens().extent, 7.0F);
    BOOST_CHECK(camera->lens().clipping == glm::vec2(0.25F, 400.0F));

    // and the projection asked for, whatever the view's
    BOOST_CHECK(v3d::editor::create_camera(view, v3d::editor::Camera::Projection::Orthographic)->lens().projection ==
        v3d::editor::Camera::Projection::Orthographic);
}

BOOST_AUTO_TEST_CASE(create_camera_from_an_unreadable_lens_test) {
    // a lens a document could not read back gives the default lens, so the camera can be saved
    v3d::type::camera::Profile view = corner(v3d::type::camera::Profile::Hand::UpCrossDirection);
    view.fov(200.0F);
    const boost::shared_ptr<v3d::editor::Camera> camera =
        v3d::editor::create_camera(view, v3d::editor::Camera::Projection::Orthographic);
    BOOST_CHECK(camera->lens().valid());
    BOOST_CHECK_EQUAL(camera->lens().fov, v3d::editor::Camera::defaultFov);
    BOOST_CHECK(camera->lens().projection == v3d::editor::Camera::Projection::Orthographic);
}

BOOST_AUTO_TEST_CASE(profile_of_a_camera_test) {
    for (const v3d::type::camera::Profile::Hand hand :
        { v3d::type::camera::Profile::Hand::UpCrossDirection, v3d::type::camera::Profile::Hand::DirectionCrossUp }) {
        const v3d::type::camera::Profile view = corner(hand);
        const boost::shared_ptr<v3d::editor::Camera> camera =
            v3d::editor::create_camera(view, v3d::editor::Camera::Projection::Perspective);

        // a camera made from a view is seen through that view again, its picture's size included
        const v3d::type::camera::Profile back = v3d::editor::profile_of(camera->lens(), camera->matrix(), view);
        v3d::type::camera::Camera expected(view);
        v3d::type::camera::Camera actual(back);
        expected.createView();
        actual.createView();
        expected.createProjection();
        actual.createProjection();
        BOOST_CHECK_SMALL(apart(expected.view(), actual.view()), 0.001F);
        BOOST_CHECK_SMALL(apart(expected.projection(), actual.projection()), 0.001F);
        BOOST_CHECK(back.size() == view.size());
        BOOST_CHECK(!back.orthographic());
    }
}

BOOST_AUTO_TEST_CASE(create_light_at_the_view_test) {
    const v3d::type::camera::Profile view = corner(v3d::type::camera::Profile::Hand::UpCrossDirection);
    const boost::shared_ptr<v3d::editor::Light> light = v3d::editor::create_light(view, v3d::editor::Light::Type::Spot);

    // a spot light at the view shines where the view looks, with RI's default spot
    BOOST_CHECK_SMALL(glm::distance(light->translation(), view.eye()), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(light->rotation() * glm::vec3(0.0F, 0.0F, 1.0F), glm::normalize(view.direction())), 0.001F);
    BOOST_CHECK(light->emission().type == v3d::editor::Light::Type::Spot);
    BOOST_CHECK_EQUAL(light->emission().cone, v3d::editor::Light::defaultCone);
}

};  // namespace
