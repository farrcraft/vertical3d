/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Jpeg.h"
#include "../Image.h"
#include "../../v3dlibs/image/reader/Jpeg.h"

#include <boost/make_shared.hpp>

using namespace odyssey::asset;
using namespace odyssey::asset::loader;

/**
 **/
Jpeg::Jpeg(const boost::shared_ptr<v3d::core::Logger>& logger) : Loader(Type::IMAGE_JPEG, logger) {

}

/**
 **/
boost::shared_ptr<Asset> Jpeg::load(std::string_view name) {
    v3d::image::reader::Jpeg reader(logger_);
    boost::shared_ptr<v3d::image::Image> image;
    image = reader.read(name);

    boost::shared_ptr<Image> asset = boost::make_shared<Image>(name, Type::IMAGE_JPEG, image);

    return asset;
}
