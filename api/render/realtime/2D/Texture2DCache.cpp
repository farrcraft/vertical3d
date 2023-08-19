/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Texture2DCache.h"

namespace v3d::render::realtime {

	/**
	 **/
	bool Texture2DCache::cache(const std::string& key, boost::shared_ptr<Texture2D> texture) {
		textures_[key] = texture;
		return true;
	}

	/**
	 **/
	boost::shared_ptr<Texture2D> Texture2DCache::fetch(const std::string& key) {
		return textures_[key];
	}

};  // namespace v3d::render::realtime
