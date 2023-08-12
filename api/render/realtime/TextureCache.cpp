/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "TextureCache.h"

namespace v3d::render::realtime {

	/**
	 **/
	bool TextureCache::cache(const std::string& key, boost::shared_ptr<Texture> texture) {
		textures_[key] = texture;
		return true;
	}

	/**
	 **/
	boost::shared_ptr<Texture> TextureCache::fetch(const std::string& key) {
		return textures_[key];
	}

};  // namespace v3d::render::realtime
