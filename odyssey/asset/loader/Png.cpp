/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Png.h"
#include "../Image.h"
#include "../Type.h"
#include "../../image/reader/Png.h"

#include <boost/make_shared.hpp>

using namespace odyssey::asset;
using namespace odyssey::asset::loader;

/**
 **/
Png::Png(const boost::shared_ptr<odyssey::engine::Logger>& logger) : Loader(Type::IMAGE_PNG, logger) {

}

/**
 **/
boost::shared_ptr<Asset> Png::load(std::string_view name) {
	odyssey::image::reader::Png reader;
	boost::shared_ptr<odyssey::image::Image> image;
	image = reader.read(name);

	boost::shared_ptr<Image> asset = boost::make_shared<Image>(name, Type::IMAGE_PNG, image);

	return asset;
}
