/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Texture.h"

namespace odyssey::render {

    /**
     **/
    Texture::Texture(boost::shared_ptr<Context> context, SDL_Surface* surface, int width, int height) :
        context_(context),
        width_(width),
        height_(height) {
        texture_ = SDL_CreateTextureFromSurface(context_->handle(), surface);
    }

    /**
     **/
    Texture::Texture(boost::shared_ptr<Context> context, int width, int height) :
        context_(context),
        width_(width),
        height_(height) {
        texture_ = SDL_CreateTexture(context_->handle(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    }

    /**
     **/
    void Texture::resize(int width, int height) {
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
    Texture::~Texture() {
        if (texture_ != nullptr) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
    }

    /**
     **/
    int Texture::width() const noexcept {
        return width_;
    }

    /**
     **/
    int Texture::height() const noexcept {
        return height_;
    }

    /**
     **/
    SDL_Texture* Texture::tex() noexcept {
        return texture_;
    }

};  // namespace odyssey::render
