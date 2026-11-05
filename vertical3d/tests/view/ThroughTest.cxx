/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/Through.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cmath>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::view::ViewPort;

/**
 * A scene of two cameras with a mesh between them, and a picture of 400 by 200.
 **/
struct Shot final {
    boost::shared_ptr<v3d::editor::scene::Scene> scene = boost::make_shared<v3d::editor::scene::Scene>();
    boost::shared_ptr<v3d::editor::scene::item::Camera> first = boost::make_shared<v3d::editor::scene::item::Camera>();
    boost::shared_ptr<v3d::editor::scene::item::Mesh> mesh =
        boost::make_shared<v3d::editor::scene::item::Mesh>(boost::make_shared<v3d::brep::BRep>());
    boost::shared_ptr<v3d::editor::scene::item::Camera> second = boost::make_shared<v3d::editor::scene::item::Camera>();

    Shot() {
        first->name("Wide");
        second->name("Close");
        second->translation(glm::vec3(0.0F, 2.0F, -5.0F));
        scene->add(first);
        scene->add(mesh);
        scene->add(second);
        v3d::editor::scene::RenderSettings settings = scene->renderSettings();
        settings.size = glm::uvec2(400, 200);
        scene->renderSettings(settings);
    }
};

/**
 * A free view of a size, starting from a perspective profile.
 **/
boost::shared_ptr<ViewPort> sized(float width, float height) {
    v3d::type::camera::Profile profile("Perspective");
    profile.fov(50.0F);
    const boost::shared_ptr<ViewPort> made = boost::make_shared<ViewPort>("view", profile);
    made->resize(glm::vec4(0.0F, 0.0F, width, height));
    return made;
}

BOOST_AUTO_TEST_SUITE(through_test)

/**
 * The frame is the largest box of the picture's shape that fits the view, centred. A view wider
 * than the picture shows the lens as it is. A view taller than it widens the lens by as much as
 * the view is taller than the frame.
 **/
BOOST_AUTO_TEST_CASE(a_camera_is_framed_as_the_picture) {
    v3d::editor::scene::item::Camera::Lens lens;
    lens.fov = 40.0F;
    lens.extent = 3.0F;

    const boost::shared_ptr<ViewPort> wide = sized(800.0F, 200.0F);
    const v3d::editor::view::Framed across = v3d::editor::view::framed(lens, glm::mat4(1.0F), wide->camera()->profile(), glm::uvec2(400, 200));
    BOOST_CHECK(across.frame == glm::vec4(200.0F, 0.0F, 400.0F, 200.0F));
    BOOST_CHECK_CLOSE(across.profile.fov(), 40.0F, 0.01F);

    const boost::shared_ptr<ViewPort> tall = sized(400.0F, 400.0F);
    const v3d::editor::view::Framed down = v3d::editor::view::framed(lens, glm::mat4(1.0F), tall->camera()->profile(), glm::uvec2(400, 200));
    BOOST_CHECK(down.frame == glm::vec4(0.0F, 100.0F, 400.0F, 200.0F));
    const float widened = glm::degrees(2.0F * std::atan(std::tan(glm::radians(20.0F)) * 2.0F));
    BOOST_CHECK_CLOSE(down.profile.fov(), widened, 0.01F);
    BOOST_CHECK_CLOSE(down.profile.orthoZoom(), 6.0F, 0.01F);
    BOOST_CHECK(down.profile.size() == glm::uvec2(400, 400));
}

/**
 * A view looks through the camera it names. One that follows the scene's camera looks through
 * the first selected camera, else the first camera in the scene, and a free view through none.
 **/
BOOST_AUTO_TEST_CASE(a_view_looks_through_its_camera) {
    const Shot shot;
    BOOST_CHECK(!v3d::editor::view::lookedThrough(*shot.scene, ViewPort::Through{}));
    BOOST_CHECK(v3d::editor::view::lookedThrough(*shot.scene, { .camera = shot.second->documentId(), .scene = false, .locked = false }) == shot.second);
    BOOST_CHECK(!v3d::editor::view::lookedThrough(*shot.scene, { .camera = shot.mesh->documentId(), .scene = false, .locked = false }));

    const ViewPort::Through follows{ .camera = 0, .scene = true, .locked = false };
    BOOST_CHECK(v3d::editor::view::lookedThrough(*shot.scene, follows) == shot.first);
    shot.second->selected(true);
    BOOST_CHECK(v3d::editor::view::lookedThrough(*shot.scene, follows) == shot.second);
}

/**
 * Following a view sets it from its camera, names it for the camera and frames it. A view whose
 * camera has gone is freed where it was, as a view named Perspective, and a view that follows the
 * scene's camera in a scene with none keeps its own camera.
 **/
BOOST_AUTO_TEST_CASE(a_view_follows_its_camera) {
    const Shot shot;
    const boost::shared_ptr<ViewPort> view = sized(400.0F, 400.0F);
    view->through({ .camera = shot.second->documentId(), .scene = false, .locked = false });
    BOOST_CHECK(!view->free());
    v3d::editor::view::follow(*shot.scene, view.get());
    BOOST_CHECK_EQUAL(view->camera()->profile().name(), "Close");
    BOOST_CHECK(view->camera()->profile().eye() == glm::vec3(0.0F, 2.0F, -5.0F));
    BOOST_REQUIRE(view->frame());
    BOOST_CHECK(*view->frame() == glm::vec4(0.0F, 100.0F, 400.0F, 200.0F));
    BOOST_CHECK_EQUAL(view->looking(), shot.second->documentId());

    shot.scene->remove(shot.second->id());
    v3d::editor::view::follow(*shot.scene, view.get());
    BOOST_CHECK(view->free());
    BOOST_CHECK(!view->frame());
    BOOST_CHECK_EQUAL(view->looking(), 0);
    BOOST_CHECK_EQUAL(view->camera()->profile().name(), "Perspective");
    BOOST_CHECK(view->camera()->profile().eye() == glm::vec3(0.0F, 2.0F, -5.0F));

    const v3d::editor::scene::Scene empty;
    const boost::shared_ptr<ViewPort> waiting = sized(400.0F, 400.0F);
    waiting->through({ .camera = 0, .scene = true, .locked = false });
    v3d::editor::view::follow(empty, waiting.get());
    BOOST_CHECK(!waiting->free());
    BOOST_CHECK(!waiting->frame());
    BOOST_CHECK_EQUAL(waiting->camera()->profile().name(), "Perspective");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
