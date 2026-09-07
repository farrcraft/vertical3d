/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AppEngine.h"

#include <api/engine/Feature.h>
#include <api/render/realtime/Pass.h>

#include <string>

#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

AppEngine::AppEngine(const std::string& path) : v3d::engine::Engine(path) {
}

bool AppEngine::initialize() {
    // Feature::Config is what reads data/config.json, and through it data/window.json. Without
    // it the window is created at its own default size rather than at the one configured.
    if (!Engine::initialize(static_cast<int>(
        v3d::engine::Feature::Window |
        v3d::engine::Feature::KeyboardInput |
        v3d::engine::Feature::Config))) {
        return false;
    }

    window_->caption("vertical3d starter");

    renderer_ = boost::make_shared<v3d::render::realtime::Engine3D>(logger_, assetManager_, &registry_);
    if (!renderer_->initialize(window())) {
        return false;
    }
    renderer_->clearColour(glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));

    return true;
}

bool AppEngine::tick(unsigned int /* delta */) {
    return true;
}

bool AppEngine::render() {
    // beginFrame is false while the window has no area: it has presented the frame empty,
    // because a canvas with no area has no projection to build geometry against
    glm::ivec2 size;
    if (!renderer_->beginFrame(&size)) {
        return true;
    }
    if (canvas_.width() != static_cast<uint32_t>(size.x) || canvas_.height() != static_cast<uint32_t>(size.y)) {
        canvas_.resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    }

    canvas_.clear();
    canvas_.rect(glm::vec2(40.0f, 40.0f),
        glm::vec2(static_cast<float>(size.x) - 40.0f, static_cast<float>(size.y) - 40.0f),
        glm::vec4(0.9f, 0.4f, 0.2f, 1.0f));

    boost::shared_ptr<v3d::render::realtime::Pass> pass = renderer_->frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    renderer_->quads()->submit(canvas_, pass.get());

    renderer_->renderFrame();
    return true;
}

bool AppEngine::shutdown() {
    // the renderer is torn down before the base class runs: the context owns the device that
    // holds the window's surface alive, and Window::destroy unloads the vulkan library
    if (renderer_) {
        renderer_->shutdown();
    }
    return Engine::shutdown();
}
