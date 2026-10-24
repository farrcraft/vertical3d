/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/view/ViewPort.h>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

BOOST_AUTO_TEST_CASE(viewport_look_test) {
    v3d::type::camera::Profile start("Perspective");
    start.orthographic(false);
    v3d::editor::ViewPort view("Perspective", start);
    view.resize(glm::vec4(10.0F, 20.0F, 400.0F, 200.0F));
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view.camera();

    v3d::type::camera::Profile front("Front");
    front.orthographic(true);
    front.eye(glm::vec3(0.0F, 0.0F, -10.0F));
    front.lookat(glm::vec3(0.0F));
    front.pixelAspect(1.33F);
    view.look(front);

    // the view shows the new profile through the same camera, at its own size
    BOOST_CHECK(view.camera() == camera);
    BOOST_CHECK_EQUAL(camera->profile().name(), "Front");
    BOOST_CHECK(camera->orthographic());
    BOOST_CHECK_SMALL(glm::distance(camera->profile().eye(), glm::vec3(0.0F, 0.0F, -10.0F)), 0.0001F);
    BOOST_CHECK(camera->profile().size() == glm::uvec2(400U, 200U));
    BOOST_CHECK_CLOSE(camera->profile().pixelAspect(), 2.0F, 0.0001F);
    BOOST_CHECK(view.region() == glm::vec4(10.0F, 20.0F, 400.0F, 200.0F));
}

};  // namespace
