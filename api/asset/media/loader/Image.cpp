/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Image.h"

#include <api/asset/media/kind/Image.h>
#include <api/image/Factory.h>

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::asset::media::loader {

/**
 **/
Image::Image(Type type, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(type, logger) {
}

/**
 **/
boost::shared_ptr<Asset> Image::load(std::string_view name) {
    logger_->get()->info("Looking for image asset at: {}", name);
    v3d::image::Factory factory(logger_);
    boost::shared_ptr<v3d::image::Image> image = factory.read(name);
    // an asset holding no image is indistinguishable from a loaded one until a
    // consumer dereferences it, so a read that failed comes back as no asset at all
    if (!image) {
        logger_->get()->error("Could not read image asset: {}", name);
        return boost::shared_ptr<Asset>();
    }
    return boost::make_shared<kind::Image>(std::string(name), type(), image);
}
};  // namespace v3d::asset::media::loader
