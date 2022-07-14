/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Asset.h"
#include "Type.h"
#include "../../v3dlibs/core/Logger.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::asset {
	/**
	 * The base interface for asset loaders
	 **/
	class Loader {
	public:
		/**
		 * Get the asset type
		 * 
		 * @return Type
		 **/
		Type type() const;

		/**
		 * Load an asset
		 * 
		 * @param name The name of the asset to be loaded
		 **/
		virtual boost::shared_ptr<Asset> load(std::string_view name) = 0;

	protected:
		/**
		 * Default constructor
		 *
		 * @param Type t The asset type this loader provides
		 **/
		Loader(Type t, const boost::shared_ptr<v3d::core::Logger>& logger);

	protected:
		boost::shared_ptr<v3d::core::Logger> logger_;

	private:
		Type type_;
	};
};
