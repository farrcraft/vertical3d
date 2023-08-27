/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Jpeg.h"
#include "../Image.h"
#include "../../image/reader/Jpeg.h"

#include <boost/make_shared.hpp>


namespace v3d::asset::loader {

    /**
     **/
    Jpeg::Jpeg(Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::IMAGE_JPEG, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> Jpeg::load(std::string_view name) {
        v3d::image::reader::Jpeg reader(logger_);
        boost::shared_ptr<v3d::image::Image> image;
        image = reader.read(name);

        boost::shared_ptr<Image> asset = boost::make_shared<Image>(std::string(name), Type::IMAGE_JPEG, image);

        return asset;
    }

};  // namespace v3d::asset::loader
