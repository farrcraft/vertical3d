/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Renderer.h>

#include <boost/test/unit_test.hpp>

/**
 * activeRenderContext hands back a reference into the stack, so a caller writing an option
 * through it is writing the context the renderer holds rather than a copy.
 **/
BOOST_AUTO_TEST_CASE(renderer_active_context_test) {
    v3d::moya::Renderer renderer;
    renderer.createRenderContext("default context");

    renderer.activeRenderContext().imageResolution(1280, 960, 1.0f);

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 1280u);
}

/**
 * The stack is what RiBegin and RiEnd push and pop: destroying the active context uncovers
 * the one below it with the options it was left holding.
 **/
BOOST_AUTO_TEST_CASE(renderer_context_stack_test) {
    v3d::moya::Renderer renderer;

    renderer.createRenderContext("outer");
    renderer.activeRenderContext().imageResolution(800, 600, 1.0f);

    renderer.createRenderContext("inner");
    renderer.activeRenderContext().imageResolution(1280, 960, 1.0f);
    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 1280u);

    renderer.destroyActiveRenderContext();
    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 800u);
}

/**
 * A renderer with no context builds one on demand rather than answering with nothing, which
 * is what makes every option call safe before RiBegin.
 **/
BOOST_AUTO_TEST_CASE(renderer_implicit_context_test) {
    v3d::moya::Renderer renderer;

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 320u);

    renderer.activeRenderContext().imageResolution(1280, 960, 1.0f);
    renderer.destroyActiveRenderContext();

    BOOST_TEST(renderer.activeRenderContext().imageWidth() == 320u);
}
