/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Blit2DTexture.h"

namespace v3d::render::realtime::operation {

/**
    **/
Blit2DTexture::Blit2DTexture(boost::shared_ptr<Texture2D> source, boost::shared_ptr<Texture2D> destination) :
    source_(source),
    destination_(destination) {
}

/**
    **/
bool Blit2DTexture::run(boost::shared_ptr<Context2D> context) {
    // Now render to the texture
    SDL_SetRenderTarget(context->handle(), destination_->tex());
    SDL_RenderClear(context->handle());

    // set the target dimensions to match the source
    SDL_FRect dest = { .x = 0.0f, .y = 0.0f, .w = static_cast<float>(source_->width()), .h = static_cast<float>(source_->height()) };

    SDL_RenderTexture(context->handle(), source_->tex(), nullptr, &dest);
    // Detach the texture
    SDL_SetRenderTarget(context->handle(), nullptr);

    return true;
}

};  // namespace v3d::render::realtime::operation
