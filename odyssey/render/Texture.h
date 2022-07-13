/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"

#include <boost/shared_ptr.hpp>
#include <SDL.h>

namespace odyssey::render {
	/**
	 * Represents an SDL Texture
	 **/
	class Texture {
	public:
		/**
		 * Create a texture from a surface 
		 **/
		Texture(boost::shared_ptr<Context> context, SDL_Surface* surface, int width, int height);

		/**
		 * Create an empty texture with given dimensions 
		 **/
		Texture(boost::shared_ptr<Context> context, int width, int height);

		/**
		 **/
		~Texture();

		/**
		 **/
		void resize(int width, int height);

		/**
		 **/
		int width() const noexcept;

		/**
		 **/
		int height() const noexcept;

		/**
		 **/
		SDL_Texture* tex() noexcept;

	private:
		boost::shared_ptr<Context> context_;
		SDL_Texture* texture_;
		int width_;
		int height_;
	};

};
