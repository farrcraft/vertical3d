/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Renderer.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * active() returns a reference into the stack. An option written through it is written to the
 * context the renderer holds, not to a copy.
 **/
BOOST_AUTO_TEST_CASE(renderer_active_context_test) {
    v3d::moya::Renderer renderer;
    renderer.begin("default context");

    renderer.active().context().imageResolution({1280, 960}, 1.0F);

    BOOST_TEST(renderer.active().context().imageWidth() == 1280U);
}

/**
 * begin() and end() push and pop the stack, as RiBegin and RiEnd do. Ending the active handler
 * makes the one below it active, with the options it held.
 **/
BOOST_AUTO_TEST_CASE(renderer_context_stack_test) {
    v3d::moya::Renderer renderer;

    renderer.begin("outer");
    renderer.active().context().imageResolution({800, 600}, 1.0F);

    renderer.begin("inner");
    renderer.active().context().imageResolution({1280, 960}, 1.0F);
    BOOST_TEST(renderer.active().context().imageWidth() == 1280U);

    renderer.end();
    BOOST_TEST(renderer.active().context().imageWidth() == 800U);
}

/**
 * A renderer with no handler builds one on demand rather than returning nothing, so every
 * option call is safe before RiBegin.
 **/
BOOST_AUTO_TEST_CASE(renderer_implicit_context_test) {
    v3d::moya::Renderer renderer;

    BOOST_TEST(renderer.active().context().imageWidth() == 320U);

    renderer.active().context().imageResolution({1280, 960}, 1.0F);
    renderer.end();

    BOOST_TEST(renderer.active().context().imageWidth() == 320U);
}

/**
 * Ending the active handler with none on the stack does nothing, as RiEnd outside any RiBegin
 * does. The stack is still usable after it.
 **/
BOOST_AUTO_TEST_CASE(renderer_destroy_with_no_context_test) {
    v3d::moya::Renderer renderer;
    renderer.end();

    renderer.begin("only");
    renderer.active().context().imageResolution({32, 16}, 1.0F);
    renderer.end();
    renderer.end();

    BOOST_TEST(renderer.active().context().imageWidth() == 320U);
}

/**
 * A handler stays where it is while others are pushed above it, so a pointer to it, which
 * RiGetContext hands out, stays good. activate() makes it the active handler again.
 **/
BOOST_AUTO_TEST_CASE(renderer_a_context_survives_a_push_test) {
    v3d::moya::Renderer renderer;
    renderer.begin("outer");
    v3d::moya::RIBHandler * outer = &renderer.active();
    outer->context().imageResolution({32, 16}, 1.0F);
    for (int i = 0; i < 8; i++) {
        renderer.begin("inner");
    }

    BOOST_TEST(outer->context().imageWidth() == 32U);
    BOOST_TEST(renderer.activate(outer));
    BOOST_TEST(&renderer.active() == outer);

    const v3d::moya::RIBHandler stranger("stranger");
    BOOST_TEST(!renderer.activate(&stranger));
    BOOST_TEST(&renderer.active() == outer);
}

};  // namespace
