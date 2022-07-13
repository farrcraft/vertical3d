/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>

namespace odyssey::component {
	/**
	 **/
	class Position final {
	public:
		Position(const int x, const int y) noexcept;

		/**
		 * Move constructor
		 **/
		Position(Position&&) noexcept;

		/**
		 * Default destructor
		 **/
		~Position() noexcept = default;

		/**
		 * Move assignment
		 **/
		Position& operator=(Position&&) noexcept;
	private:
		glm::ivec2 position_;
	};
};
