/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

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

};  // namespace
