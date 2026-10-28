/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Curve.h>
#include <api/brep/Primitives.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A cube in the middle of the default front view.
 **/
struct Cube {
    Cube() :
        mesh(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())) {
        scene.add(mesh);
        settings.size = glm::uvec2(64, 48);
        settings.samples = 1;
    }

    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::editor::Mesh> mesh;
    v3d::type::camera::Profile view{ "front" };
    v3d::editor::RenderSettings settings;
};

/**
 * Wait for a render to end, for as long as a small one could take on a slow machine.
 **/
v3d::editor::Delegate::Status settle(const v3d::editor::Delegate& delegate) {
    constexpr std::chrono::milliseconds pause{ 10 };
    constexpr int tries = 3000;
    for (int attempt = 0; attempt < tries; attempt++) {
        const v3d::editor::Delegate::Status status = delegate.status();
        if (status.state != v3d::editor::Delegate::State::Rendering) {
            return status;
        }
        std::this_thread::sleep_for(pause);
    }
    return delegate.status();
}

/**
 * The red of one pixel of an rgb picture.
 **/
unsigned char red(const v3d::image::Image& image, unsigned int column, unsigned int row) {
    return image[((row * image.width()) + column) * 3];
}

BOOST_FIXTURE_TEST_CASE(delegate_renders_test, Cube) {
    v3d::editor::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(delegate.status().state == v3d::editor::Delegate::State::Idle);
    BOOST_CHECK(!delegate.picture());

    // a render runs off the caller's thread and leaves its picture in memory, at the size
    // the settings give. The cube faces the camera's own light, so its middle is lit
    delegate.render(scene, view, settings);
    const v3d::editor::Delegate::Status status = settle(delegate);
    BOOST_REQUIRE(status.state == v3d::editor::Delegate::State::Finished);
    BOOST_CHECK_EQUAL(status.done, 1.0F);
    BOOST_CHECK_EQUAL(status.pictures, 1U);
    const boost::shared_ptr<v3d::image::Image> picture = delegate.picture();
    BOOST_REQUIRE(picture);
    BOOST_CHECK_EQUAL(picture->width(), 64U);
    BOOST_CHECK_EQUAL(picture->height(), 48U);
    BOOST_CHECK_GT(red(*picture, 32, 24), 0);
    BOOST_CHECK_EQUAL(red(*picture, 1, 1), 0);
}

BOOST_FIXTURE_TEST_CASE(delegate_update_test, Cube) {
    v3d::editor::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());

    // there is nothing to bring up to date before a first render
    BOOST_CHECK(!delegate.update(scene, view, settings));
    delegate.render(scene, view, settings);
    settle(delegate);

    // the same scene, camera and settings make the same picture, so nothing renders again
    BOOST_CHECK(!delegate.update(scene, view, settings));

    // an item that moves, a camera that moves and settings that change each render again
    mesh->translate(glm::vec3(0.25F, 0.0F, 0.0F));
    BOOST_CHECK(delegate.update(scene, view, settings));
    settle(delegate);
    BOOST_CHECK(!delegate.update(scene, view, settings));
    view.eye(glm::vec3(0.1F, 0.0F, -1.0F));
    BOOST_CHECK(delegate.update(scene, view, settings));
    settle(delegate);
    settings.samples = 2;
    BOOST_CHECK(delegate.update(scene, view, settings));
    BOOST_CHECK(settle(delegate).state == v3d::editor::Delegate::State::Finished);
    BOOST_CHECK_EQUAL(delegate.status().pictures, 4U);
}

BOOST_FIXTURE_TEST_CASE(delegate_update_frame_test, Cube) {
    v3d::editor::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    v3d::animation::Curve curve;
    curve.set(1.0F, 0.0F);
    curve.set(2.0F, 0.25F);
    scene.curve(mesh->id(), "translation.x", curve);
    delegate.render(scene, view, settings);
    settle(delegate);

    // a frame where a curve moves an item renders again, though no item has changed
    const std::uint64_t revision = mesh->revision();
    scene.frame(2);
    BOOST_CHECK_EQUAL(mesh->revision(), revision);
    BOOST_CHECK(delegate.update(scene, view, settings));
    settle(delegate);
    BOOST_CHECK(!delegate.update(scene, view, settings));
}

BOOST_FIXTURE_TEST_CASE(delegate_cancel_test, Cube) {
    v3d::editor::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    delegate.render(scene, view, settings);
    settle(delegate);
    const boost::shared_ptr<v3d::image::Image> finished = delegate.picture();

    // a render far too large to finish at once is stopped where it is, and the last
    // finished picture stays the one there is. It traces rays, which keep nothing per sample,
    // so a large picture is slow without being large in memory
    settings.size = glm::uvec2(2048, 2048);
    settings.samples = 8;
    settings.hider = v3d::editor::RenderSettings::Hider::RayTrace;
    delegate.render(scene, view, settings);
    delegate.cancel();
    const v3d::editor::Delegate::Status status = delegate.status();
    BOOST_CHECK(status.state == v3d::editor::Delegate::State::Stopped);
    BOOST_CHECK_EQUAL(status.pictures, 1U);
    BOOST_CHECK(delegate.picture() == finished);
}

};  // namespace
