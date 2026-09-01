/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Tga.h"

#include <string>

#include "../Image.h"
#include "../Type.h"
#include "../../image/reader/Tga.h"

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

    /**
     **/
    Tga::Tga(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::ImageTga, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> Tga::load(std::string_view name) {
        v3d::image::reader::Tga reader(logger_);
        boost::shared_ptr<v3d::image::Image> image;
        image = reader.read(name);

        boost::shared_ptr<Image> asset = boost::make_shared<Image>(std::string(name), Type::ImageTga, image);

        return asset;
    }
};  // namespace v3d::asset::loader
