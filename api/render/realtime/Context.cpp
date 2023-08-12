/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Context.h"

namespace v3d::render::realtime {

    /**
     **/
    Context::Context(boost::shared_ptr<v3d::ui::Window> window) {
        renderer_ = SDL_CreateRenderer(window->sdl(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        if (renderer_ == NULL) {
            throw std::invalid_argument(SDL_GetError());
        }
    }

    /**
     **/
    Context::~Context() {
        if (renderer_ != NULL) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = NULL;
        }
    }

    /**
     **/
    SDL_Renderer* Context::handle() {
        return renderer_;
    }

};  // namespace v3d::render::realtime
