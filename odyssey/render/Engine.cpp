/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine.h"
#include "../engine/Unit.h"

#include <boost/make_shared.hpp>

using namespace odyssey::render;

/**
 **/
Engine::Engine(const boost::shared_ptr<odyssey::engine::Logger> &logger, const boost::shared_ptr<odyssey::asset::Manager> &assetManager) :
	logger_(logger),
	assetManager_(assetManager) {
	textureCache_ = boost::make_shared<TextureCache>();
}

/**
 **/
Engine::~Engine() {

}

/**
 **/
bool Engine::initialize(const boost::shared_ptr <odyssey::ui::Window> &window) {
	window_ = window;
	context_ = boost::make_shared<Context>(window_);
	scene_ = boost::make_shared<Scene>(context_);

	// we need to determine the scale factor
	// use tile dimension * # of tiles on the screen for that dimension = reference screen dimension for the logical size
	int width = odyssey::engine::unit::tile_width * odyssey::engine::unit::screen_tile_width;
	int height = odyssey::engine::unit::tile_height * odyssey::engine::unit::screen_tile_height;
	SDL_RenderSetLogicalSize(context_->handle(), width, height);

	backBuffer_ = boost::make_shared<Texture>(context_, width, height);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	return true;
}

/**
 **/
bool Engine::shutdown() {

	return true;
}

/**
 **/
void Engine::resize(int width, int height) {
	window_->resize(width, height);
	//backBuffer_ = boost::make_shared<Texture>(context_, width, height);
}

/**
 **/
void Engine::renderFrame() {
	boost::shared_ptr<Frame> frame = scene_->collect();
	frame->draw();

	// flip backbuffer
	SDL_RenderClear(context_->handle());
	SDL_RenderCopyEx(context_->handle(), backBuffer_->tex(), nullptr, nullptr, 0, nullptr, SDL_FLIP_VERTICAL);
	SDL_RenderPresent(context_->handle());
}

/**
 **/
boost::shared_ptr<odyssey::asset::Manager> Engine::assetManager() {
	return assetManager_;
}

/**
 **/
boost::shared_ptr<TextureCache> Engine::textureCache() {
	return textureCache_;
}

/**
 **/
boost::shared_ptr<Context> Engine::context() {
	return context_;
}

/**
 **/
boost::shared_ptr<Texture> Engine::backBuffer() {
	return backBuffer_;
}

/**
 **/
boost::shared_ptr<Scene> Engine::scene() {
	return scene_;
}
