/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine2D.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
    /**
     **/
    Engine2D::Engine2D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry& registry) :
        Engine(logger, assetManager, registry) {
    }

    /**
     **/
    bool Engine2D::initialize(const boost::shared_ptr <Window2D>& window) {
        Engine::initialize(window);
        textureCache_ = boost::make_shared<v3d::render::realtime::Texture2DCache>();
        context_ = boost::make_shared<v3d::render::realtime::Context2D>(window);
        scene_ = boost::make_shared<Scene2D>(context_);

        SDL_RenderSetLogicalSize(context_->handle(), window->logicalWidth(), window->logicalHeight());

        backBuffer_ = boost::make_shared<v3d::render::realtime::Texture2D>(context_, window->logicalWidth(), window->logicalHeight());

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

        return true;
    }

    /**
     **/
    void Engine2D::renderFrame() {
        boost::shared_ptr<Frame> frame = scene_->collect();
        frame->draw();

        // flip backbuffer
        SDL_RenderClear(context_->handle());
        SDL_RenderCopyEx(context_->handle(), backBuffer_->tex(), nullptr, nullptr, 0, nullptr, SDL_FLIP_VERTICAL);
        SDL_RenderPresent(context_->handle());
    }

    /**
     **/
    boost::shared_ptr<Texture2DCache> Engine2D::textureCache() {
        return textureCache_;
    }

    /**
     **/
    boost::shared_ptr<Context2D> Engine2D::context() {
        return context_;
    }

    /**
     **/
    boost::shared_ptr<Texture2D> Engine2D::backBuffer() {
        return backBuffer_;
    }

    /**
     **/
    boost::shared_ptr<Scene2D> Engine2D::scene() {
        return scene_;
    }

};  // namespace v3d::render::realtime
