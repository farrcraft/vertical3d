/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Texture2D.h"

namespace v3d::render::realtime {

    /**
     **/
    Texture2D::Texture2D(boost::shared_ptr<Context2D> context, SDL_Surface* surface, int width, int height) :
        context_(context),
        width_(width),
        height_(height) {
        texture_ = SDL_CreateTextureFromSurface(context_->handle(), surface);
    }

    /**
     **/
    Texture2D::Texture2D(boost::shared_ptr<Context2D> context, int width, int height) :
        context_(context),
        width_(width),
        height_(height) {
        texture_ = SDL_CreateTexture(context_->handle(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    }

    /**
     **/
    void Texture2D::resize(int width, int height) {
        SDL_Texture* resized = SDL_CreateTexture(context_->handle(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        SDL_SetRenderTarget(context_->handle(), resized);
        SDL_Rect dest = { .x = 0, .y = 0, .w = width, .h = height };
        SDL_RenderCopy(context_->handle(), texture_, nullptr, &dest);
        SDL_SetRenderTarget(context_->handle(), nullptr);
        SDL_DestroyTexture(texture_);
        width_ = width;
        height_ = height;
        texture_ = resized;
    }

    /**
     **/
    Texture2D::~Texture2D() {
        if (texture_ != nullptr) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
    }

    /**
     **/
    int Texture2D::width() const noexcept {
        return width_;
    }

    /**
     **/
    int Texture2D::height() const noexcept {
        return height_;
    }

    /**
     **/
    SDL_Texture* Texture2D::tex() noexcept {
        return texture_;
    }

};  // namespace v3d::render::realtime
