/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace odyssey::event {

	/**
	 **/
	class Key {
	public:
		/**
		 **/
		Key(const std::string_view &name, bool pressed) noexcept;

		/**
		 **/
		bool pressed() const noexcept;

		/**
		 **/
		std::string_view name() const noexcept;
	private:
		std::string name_;
		bool pressed_;
	};
};
