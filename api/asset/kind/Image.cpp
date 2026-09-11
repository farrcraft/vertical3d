/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Image.h"

#include <string>

namespace v3d::asset::kind {
/**
 **/
Image::Image(const std::string& name, Type t, const boost::shared_ptr<v3d::image::Image>& img) :
    Asset(name, t),
    image_(img) {
}

/**
 **/
boost::shared_ptr<v3d::image::Image> Image::image() {
    return image_;
}

};  // namespace v3d::asset::kind
