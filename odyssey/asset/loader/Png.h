/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Loader.h"

namespace odyssey::asset::loader {
	/**
	 **/
	class Png final : public Loader {
	public:
		/**
		 **/
		Png(const boost::shared_ptr<odyssey::engine::Logger>& logger);

		/**
		 **/
		boost::shared_ptr<Asset> load(std::string_view name);
	};
};
