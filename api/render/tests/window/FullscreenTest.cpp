/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <SDL3/SDL_video.h>

#include <boost/test/unit_test.hpp>

#include "Windowed.h"

namespace {

/**
 * Wait for the platform to finish a change of window state, and give the window the size it
 * now has, as the engine does when the resize event arrives.
 **/
void settle(const v3d::test::Windowed& windowed) {
    SDL_SyncWindow(windowed.window->sdl());
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(windowed.window->sdl(), &width, &height);
    windowed.window->resize(glm::ivec2(width, height));
}

/**
 * The window fills its display and comes back, and the frame loop draws through both changes
 * without a validation error, rebuilding the swapchain for each new size.
 **/
BOOST_AUTO_TEST_CASE(window_goes_full_screen_and_back_test) {
    const v3d::test::Windowed windowed;
    BOOST_TEST(!windowed.window->fullscreen());

    BOOST_REQUIRE(windowed.window->fullscreen(true));
    settle(windowed);
    BOOST_TEST(windowed.window->fullscreen());
    windowed.engine->renderFrame();
    windowed.engine->renderFrame();

    BOOST_REQUIRE(windowed.window->fullscreen(false));
    settle(windowed);
    BOOST_TEST(!windowed.window->fullscreen());
    windowed.engine->renderFrame();
    windowed.engine->renderFrame();

    BOOST_TEST(windowed.silent());
}

};  // namespace
