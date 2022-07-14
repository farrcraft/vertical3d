/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Image.h"

namespace odyssey::asset {
    /**
     **/
    Image::Image(std::string_view name, Type t, boost::shared_ptr<v3d::image::Image> img) :
        Asset(name, t),
        image_(img) {

    }

    /**
     **/
    boost::shared_ptr<v3d::image::Image> Image::image() {
        return image_;
    }

};  // namespace odyssey::asset
