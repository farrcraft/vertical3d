/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "TextureCache.h"

using namespace odyssey::render;

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