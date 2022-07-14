/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Loader.h"

namespace odyssey::asset::loader {
	/**
	 **/
	class Jpeg final : public Loader {
	public:
		/**
		 **/
		Jpeg(const boost::shared_ptr<v3d::core::Logger> &logger);

		/**
		 **/
		boost::shared_ptr<Asset> load(std::string_view name);
	};
};
