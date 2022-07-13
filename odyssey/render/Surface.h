/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../image/Image.h"

#include <boost/shared_ptr.hpp>
#include <SDL.h>

namespace odyssey::render {
	/**
	 * Represents an SDL surface
	 **/
	class Surface {
	public:
		/**
		 * Create an SDL surface from an image object
		 **/
		Surface(boost::shared_ptr<odyssey::image::Image> image);

		/**
		 **/
		~Surface();

		/**
		 **/
		SDL_Surface* surface();

	private:
		SDL_Surface* surface_;
	};
};
