/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Context.h"

using namespace odyssey::render;

/**
 **/
Context::Context(boost::shared_ptr<odyssey::ui::Window> window) {
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