/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../ui/Window.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
	/**
	 **/
	class Context {
	public:
		/**
		 **/
		Context(boost::shared_ptr<odyssey::ui::Window> window);

		/**
		 **/
		~Context();

		/**
		 **/
		SDL_Renderer* handle();

	private:
		SDL_Renderer* renderer_;
	};
};
