/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Texture.h"

#include <unordered_map>

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
	/**
	 **/
	class TextureCache {
	public:
		/**
		 **/
		bool cache(const std::string& key, boost::shared_ptr<Texture> texture);
		
		/**
		 **/
		boost::shared_ptr<Texture> fetch(const std::string& key);

	private:
		std::unordered_map<std::string, boost::shared_ptr<Texture>> textures_;
	};
};
