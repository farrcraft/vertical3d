/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Asset.h"

#include <vector>

namespace odyssey::asset {
	/**
	 **/
	class Cache {
	public:
		/**
		 **/
		Cache(Type t);

	private:
		std::vector<Asset> assets_;
		Type type_;
	};
};
