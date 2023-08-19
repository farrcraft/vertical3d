/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context2D.h"

#include <boost/shared_ptr.hpp>
#include <SDL.h>

namespace v3d::render::realtime {
	/**
	 * Represents an SDL Texture
	 **/
	class Texture2D {
	public:
		/**
		 * Create a texture from a surface 
		 **/
		Texture2D(boost::shared_ptr<Context2D> context, SDL_Surface* surface, int width, int height);

		/**
		 * Create an empty texture with given dimensions 
		 **/
		Texture2D(boost::shared_ptr<Context2D> context, int width, int height);

		/**
		 **/
		~Texture2D();

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
		boost::shared_ptr<Context2D> context_;
		SDL_Texture* texture_;
		int width_;
		int height_;
	};

};  // namespace v3d::render::realtime
