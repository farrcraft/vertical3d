/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/WindowRenderer.h>
#include <api/render/realtime/frame/Frame.h>
#include <api/render/realtime/frame/Pass.h>
#include <api/window/Window.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Windowed.h"

namespace {

/**
 * A second small window, destroyed with the case.
 **/
struct Second {
    explicit Second(const v3d::test::Windowed& windowed) :
        window(boost::make_shared<v3d::window::Window>(windowed.logger)) {
        BOOST_REQUIRE(window->create(glm::ivec2(160, 120)));
    }

    ~Second() {
        window->destroy();
    }

    Second(const Second&) = delete;
    Second& operator=(const Second&) = delete;

    boost::shared_ptr<v3d::window::Window> window;
};

/**
 * A second window is drawn by the renderer's device in the same frames as its own, and each
 * frame records and presents both without a validation message.
 **/
BOOST_AUTO_TEST_CASE(window_renderer_draws_a_second_window_test) {
    const v3d::test::Windowed windowed;
    {
        const Second second(windowed);
        const boost::shared_ptr<v3d::render::realtime::WindowRenderer::Output> output = windowed.renderer->open(second.window);
        BOOST_REQUIRE(output);
        BOOST_TEST(output->window() == second.window);
        BOOST_REQUIRE(output->frame());
        BOOST_REQUIRE_EQUAL(output->frame()->passes().size(), 1U);
        BOOST_TEST(output->frame()->passes().front()->name() == v3d::render::realtime::WindowRenderer::colourPass);

        output->frame()->pass(v3d::render::realtime::WindowRenderer::colourPass)->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
        for (int frame = 0; frame < 4; frame++) {
            windowed.renderer->renderFrame();
        }
        windowed.renderer->close(output);
    }
    // the renderer's own window goes on after the other is closed and destroyed
    windowed.renderer->renderFrame();
    BOOST_TEST(windowed.silent());
}

/**
 * Closing a window that is not open, or one already closed, changes nothing.
 **/
BOOST_AUTO_TEST_CASE(window_renderer_closes_a_window_once_test) {
    const v3d::test::Windowed windowed;
    const Second second(windowed);
    const boost::shared_ptr<v3d::render::realtime::WindowRenderer::Output> output = windowed.renderer->open(second.window);
    BOOST_REQUIRE(output);
    windowed.renderer->close(output);
    windowed.renderer->close(output);
    windowed.renderer->close(nullptr);
    windowed.renderer->renderFrame();
    BOOST_TEST(windowed.silent());
}

/**
 * A window still open when the renderer shuts down is closed by it, before the device goes.
 **/
BOOST_AUTO_TEST_CASE(window_renderer_shutdown_closes_its_windows_test) {
    const v3d::test::Windowed windowed;
    const Second second(windowed);
    BOOST_REQUIRE(windowed.renderer->open(second.window));
    windowed.renderer->renderFrame();
    windowed.renderer->shutdown();
    BOOST_TEST(windowed.silent());
}

};  // namespace
