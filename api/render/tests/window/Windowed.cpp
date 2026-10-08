/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Windowed.h"

#include <SDL3/SDL.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace v3d::test {

namespace {

/**
 * Small, and above the width Windows enforces for a window with a title bar, so the chain is
 * the size asked for.
 **/
const int windowWidth = 160;
const int windowHeight = 120;

};  // namespace

/**
 **/
Windowed::Windowed() {
    // where a captured frame is written, beside the executable
    boost::filesystem::create_directory("data_out");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL could not start its video subsystem: ") + SDL_GetError());
    }
    logger = boost::make_shared<v3d::log::Logger>();
    window = boost::make_shared<render::realtime::Window>(logger);
    if (!window->create(windowWidth, windowHeight)) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL could not create a window: ") + SDL_GetError());
    }
    engine = boost::make_shared<render::realtime::Engine3D>(logger, nullptr);
    try {
        engine->initialize(window);
    } catch (...) {
        engine.reset();
        window->destroy();
        SDL_Quit();
        throw;
    }
}

/**
 **/
Windowed::~Windowed() {
    // the engine's device holds the window's surface, so it goes first
    engine->shutdown();
    engine.reset();
    window->destroy();
    SDL_Quit();
}

/**
 **/
bool Windowed::silent() const {
    // the layer must be on for an empty log to mean anything: where it is not installed nothing
    // checked the calls, and no errors reported looks exactly like a clean run
    const boost::shared_ptr<render::realtime::vulkan::device::Instance> instance = window->instance();
    if (!instance->validating()) {
        BOOST_TEST_MESSAGE("the validation layer is not installed - this case asserts nothing");
        return false;
    }
    if (instance->errors() > 0) {
        BOOST_TEST_MESSAGE("first validation error: " << instance->firstError());
    }
    return instance->errors() == 0;
}

/**
 **/
bool windowAvailable() {
    try {
        Windowed windowed;
        return true;
    } catch (const std::exception& error) {
        // the console rather than the logger, which writes to a file beside the executable
        std::cerr << "no window to present to: " << error.what() << "\n";
        return false;
    }
}

};  // namespace v3d::test
