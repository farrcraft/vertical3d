/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Renderer.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * activeRenderContext returns a reference into the stack. An option written through it is
 * written to the context the renderer holds, not to a copy.
 **/
BOOST_AUTO_TEST_CASE(renderer_active_context_test) {
    v3d::moya::Renderer renderer;
    renderer.createRenderContext("default context");

    renderer.activeRenderContext().imageResolution({1280, 960}, 1.0F);

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 1280U);
}

/**
 * createRenderContext() and destroyActiveRenderContext() push and pop the context stack, as
 * RiBegin and RiEnd do. Destroying the active context makes the one below it active, with the
 * options it held.
 **/
BOOST_AUTO_TEST_CASE(renderer_context_stack_test) {
    v3d::moya::Renderer renderer;

    renderer.createRenderContext("outer");
    renderer.activeRenderContext().imageResolution({800, 600}, 1.0F);

    renderer.createRenderContext("inner");
    renderer.activeRenderContext().imageResolution({1280, 960}, 1.0F);
    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 1280U);

    renderer.destroyActiveRenderContext();
    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 800U);
}

/**
 * A renderer with no context builds one on demand rather than returning nothing, so every
 * option call is safe before RiBegin.
 **/
BOOST_AUTO_TEST_CASE(renderer_implicit_context_test) {
    v3d::moya::Renderer renderer;

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 320U);

    renderer.activeRenderContext().imageResolution({1280, 960}, 1.0F);
    renderer.destroyActiveRenderContext();

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 320U);
}

/**
 * Destroying the active context with none on the stack does nothing, as RiEnd outside any
 * RiBegin does. The stack is still usable after it.
 **/
BOOST_AUTO_TEST_CASE(renderer_destroy_with_no_context_test) {
    v3d::moya::Renderer renderer;
    renderer.destroyActiveRenderContext();

    renderer.createRenderContext("only");
    renderer.activeRenderContext().imageResolution({32, 16}, 1.0F);
    renderer.destroyActiveRenderContext();
    renderer.destroyActiveRenderContext();

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 320U);
}

/**
 * A context stays where it is while others are pushed above it, so a pointer to it, which
 * RiGetContext hands out, stays good. activate() makes it the active context again.
 **/
BOOST_AUTO_TEST_CASE(renderer_a_context_survives_a_push_test) {
    v3d::moya::Renderer renderer;
    renderer.createRenderContext("outer");
    v3d::moya::RenderContext * outer = &renderer.activeRenderContext();
    outer->imageResolution({32, 16}, 1.0F);
    for (int i = 0; i < 8; i++) {
        renderer.createRenderContext("inner");
    }

    BOOST_TEST(outer->imageWidth() == 32U);
    BOOST_TEST(renderer.activate(outer));
    BOOST_TEST(&renderer.activeRenderContext() == outer);

    const v3d::moya::RenderContext stranger("stranger");
    BOOST_TEST(!renderer.activate(&stranger));
    BOOST_TEST(&renderer.activeRenderContext() == outer);
}

};  // namespace
