/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "BlitTexture.h"

using namespace odyssey::render;
using namespace odyssey::render::operation;

/**
 **/
BlitTexture::BlitTexture(boost::shared_ptr<Texture> source, boost::shared_ptr<Texture> destination) :
	source_(source),
	destination_(destination) {

}

/**
 **/
bool BlitTexture::run(boost::shared_ptr<Context> context) {
	//Now render to the texture
	SDL_SetRenderTarget(context->handle(), destination_->tex());
	SDL_RenderClear(context->handle());

	// set the target dimensions to match the source
	SDL_Rect dest = { .x = 0, .y = 0, .w = source_->width(), .h = source_->height() };

	SDL_RenderCopy(context->handle(), source_->tex(), nullptr, &dest);
	//Detach the texture
	SDL_SetRenderTarget(context->handle(), nullptr);

	return true;
}
