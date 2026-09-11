/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/asset/Asset.h>
#include <api/image/Image.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::asset::kind {
/**
 **/
class Image : public Asset {
 public:
    /**
     **/
    Image(const std::string& name, Type t, const boost::shared_ptr<v3d::image::Image>& img);

    /**
     **/
    boost::shared_ptr<v3d::image::Image> image();

 private:
    boost::shared_ptr<v3d::image::Image> image_;
};
};  // namespace v3d::asset::kind
