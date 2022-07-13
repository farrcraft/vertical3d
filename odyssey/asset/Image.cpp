/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Image.h"

using namespace odyssey::asset;

/**
 **/
Image::Image(std::string_view name, Type t, boost::shared_ptr<odyssey::image::Image> img) : 
	Asset(name, t),
	image_(img) {

}

/**
 **/
boost::shared_ptr<odyssey::image::Image> Image::image() {
	return image_;
}