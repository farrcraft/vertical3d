/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Png.h"
#include "../Image.h"
#include "../Type.h"
#include "../../v3dlibs/image/reader/Png.h"

#include <boost/make_shared.hpp>

namespace odyssey::asset::loader {

    /**
     **/
    Png::Png(const boost::shared_ptr<v3d::core::Logger>& logger) : Loader(Type::IMAGE_PNG, logger) {

    }

    /**
     **/
    boost::shared_ptr<Asset> Png::load(std::string_view name) {
        v3d::image::reader::Png reader(logger_);
        boost::shared_ptr<v3d::image::Image> image;
        image = reader.read(name);

        boost::shared_ptr<Image> asset = boost::make_shared<Image>(name, Type::IMAGE_PNG, image);

        return asset;
    }
};  // namespace odyssey::asset::loader
