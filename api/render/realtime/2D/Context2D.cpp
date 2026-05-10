/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Context2D.h"

namespace v3d::render::realtime {

    /**
     **/
    Context2D::Context2D(boost::shared_ptr<Window2D> window) {
        renderer_ = SDL_CreateRenderer(window->sdl(), NULL);
        if (renderer_ == NULL) {
            throw std::invalid_argument(SDL_GetError());
        }
    }

    /**
     **/
    Context2D::~Context2D() {
        if (renderer_ != NULL) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = NULL;
        }
    }

    /**
     **/
    SDL_Renderer* Context2D::handle() {
        return renderer_;
    }

};  // namespace v3d::render::realtime
