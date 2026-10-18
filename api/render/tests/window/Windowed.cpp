/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Windowed.h"

#include <api/log/Logger.h>
#include <api/render/realtime/Engine3D.h>
#include <api/render/realtime/Window.h>
#include <api/render/realtime/vulkan/device/Instance.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include <SDL3/SDL.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace v3d::test {

namespace {

/**
 * Small, but wider than the minimum Windows enforces for a window with a title bar, so the
 * swapchain is the size requested.
 **/
const int windowWidth = 160;
const int windowHeight = 120;

};  // namespace

/**
 **/
Windowed::Windowed(render::realtime::vulkan::memory::Allocator::Kind allocations) {
    // where a captured frame is written, beside the executable
    boost::filesystem::create_directory("data_out");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL could not start its video subsystem: ") + SDL_GetError());
    }
    logger = boost::make_shared<v3d::log::Logger>();
    window = boost::make_shared<render::realtime::Window>(logger);
    if (!window->create(glm::ivec2(windowWidth, windowHeight))) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL could not create a window: ") + SDL_GetError());
    }
    engine = boost::make_shared<render::realtime::Engine3D>(logger, nullptr, VK_FORMAT_UNDEFINED, allocations);
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
    // no errors means a clean run only when the layer is on. Without the layer nothing checks
    // the calls, and every run reports no errors
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
        const Windowed windowed;
        return true;
    } catch (const std::exception& error) {
        // write to the console, not the logger, which writes to a file
        std::cerr << "no window to present to: " << error.what() << "\n";
        return false;
    }
}

};  // namespace v3d::test
