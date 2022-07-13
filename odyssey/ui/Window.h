/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../engine/Logger.h"

#include <SDL.h>

namespace odyssey::ui {

	/**
	**/
	class Window final {
	public:
		/**
		 * @param Logger* logger
		 **/
		Window(const boost::shared_ptr<odyssey::engine::Logger>& logger) noexcept;

		/**
		 * @return bool
		 **/
		bool create(int width, int height);

		/**
		 * @return void
		 **/
		void destroy();

		/**
		 **/
		SDL_Window* sdl() noexcept;

		/**
		 * Paint a surface onto the window.
		 * Typically this isn't used and texture rendering is done instead.
		 **/
		void paint(SDL_Surface *surface);

		/**
		 **/
		void resize(int width, int height) noexcept;

		/**
		 **/
		int width() const noexcept;

		/**
		 **/
		int height() const noexcept;

	private:
		SDL_Window* window_;
		SDL_Surface* surface_;
		int width_;
		int height_;
		boost::shared_ptr<odyssey::engine::Logger> logger_;
	};

};
