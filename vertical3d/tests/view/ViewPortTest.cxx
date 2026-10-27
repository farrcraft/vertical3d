/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <boost/make_shared.hpp>
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

BOOST_AUTO_TEST_CASE(viewport_shows_cameras_and_lights_test) {
    const v3d::type::camera::Profile profile("Perspective");
    v3d::editor::ViewPort view("Perspective", profile);
    view.resize(glm::vec4(0.0F, 0.0F, 400.0F, 400.0F));

    // cameras and lights are shown from the start, as meshes are
    BOOST_CHECK(view.shows(v3d::editor::ViewPort::SHOW_CAMERA));
    BOOST_CHECK(view.shows(v3d::editor::ViewPort::SHOW_LIGHT));

    v3d::editor::Scene scene;
    scene.add(boost::make_shared<v3d::editor::Camera>());
    scene.add(boost::make_shared<v3d::editor::Light>());
    view.show(v3d::editor::ViewPort::SHOW_GRID, false);
    const auto drawn = [&]() {
        v3d::render::realtime::canvas::Line canvas;
        view.draw(scene, nullptr, v3d::editor::Target(), &canvas, nullptr);
        return canvas.vertices().size() / 2;
    };

    // a camera of eleven segments and a point light of three, each behind its own flag
    BOOST_CHECK_EQUAL(drawn(), 14U);
    view.show(v3d::editor::ViewPort::SHOW_CAMERA, false);
    BOOST_CHECK_EQUAL(drawn(), 3U);
    view.show(v3d::editor::ViewPort::SHOW_LIGHT, false);
    BOOST_CHECK_EQUAL(drawn(), 0U);
}

BOOST_AUTO_TEST_CASE(viewport_display_test) {
    const v3d::type::camera::Profile profile("Perspective");
    v3d::editor::ViewPort first("first", profile);
    const v3d::editor::ViewPort second("second", profile);

    // a view shades flat, fills the front of a surface and leaves its back out
    BOOST_CHECK(first.display().shading == v3d::editor::Shading::Flat);
    BOOST_CHECK(first.display().cull == v3d::editor::ViewPort::Cull::Back);
    BOOST_CHECK(first.display().front == v3d::editor::ViewPort::Fill::Fill);

    // and each view keeps its own
    first.display().shading = v3d::editor::Shading::Smooth;
    first.display().cull = v3d::editor::ViewPort::Cull::None;
    BOOST_CHECK(second.display().shading == v3d::editor::Shading::Flat);
    BOOST_CHECK(second.display().cull == v3d::editor::ViewPort::Cull::Back);
}

};  // namespace
