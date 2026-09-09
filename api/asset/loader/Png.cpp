/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Png.h"

#include <api/asset/Image.h>
#include <api/asset/Type.h>
#include <api/image/reader/Png.h>

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

/**
 **/
Png::Png(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::ImagePng, logger) {
}

/**
 **/
boost::shared_ptr<Asset> Png::load(std::string_view name) {
    logger_->get()->info("Looking for png asset at: {}", name);
    v3d::image::reader::Png reader(logger_);
    boost::shared_ptr<v3d::image::Image> image = reader.read(name);
    // an asset holding no image is indistinguishable from a loaded one until a
    // consumer dereferences it, so a read that failed comes back as no asset at all
    if (!image) {
        logger_->get()->error("Could not read png asset: {}", name);
        return boost::shared_ptr<Asset>();
    }

    boost::shared_ptr<Image> asset = boost::make_shared<Image>(std::string(name), Type::ImagePng, image);

    return asset;
}
};  // namespace v3d::asset::loader
