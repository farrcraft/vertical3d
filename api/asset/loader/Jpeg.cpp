/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Jpeg.h"

#include <string>

#include "../Image.h"
#include "../../image/reader/Jpeg.h"

#include <boost/make_shared.hpp>


namespace v3d::asset::loader {

    /**
     **/
    Jpeg::Jpeg(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::ImageJpeg, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> Jpeg::load(std::string_view name) {
        logger_->get()->info("Looking for jpeg asset at: {}", name);
        v3d::image::reader::Jpeg reader(logger_);
        boost::shared_ptr<v3d::image::Image> image = reader.read(name);
        // an asset holding no image is indistinguishable from a loaded one until a
        // consumer dereferences it, so a read that failed comes back as no asset at all
        if (!image) {
            logger_->get()->error("Could not read jpeg asset: {}", name);
            return boost::shared_ptr<Asset>();
        }

        boost::shared_ptr<Image> asset = boost::make_shared<Image>(std::string(name), Type::ImageJpeg, image);

        return asset;
    }

};  // namespace v3d::asset::loader
