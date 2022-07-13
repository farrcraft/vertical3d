/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Type.h"

#include <string_view>

namespace odyssey::asset {
	/**
	 **/
	class Asset {
	public:
		/**
		 **/
		Asset(std::string_view name, Type t);

		/**
		 * Default destructor
		 **/
		virtual ~Asset() = default;

	protected:
		std::string_view name_;
		Type type_;
	};
};
