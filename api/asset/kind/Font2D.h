/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/asset/Asset.h>
#include <api/font/Font2D.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::asset::kind {
/**
 **/
class Font2D : public Asset {
 public:
    /**
     **/
    Font2D(const std::string& name, Type t, const boost::shared_ptr<v3d::font::Font2D>& font);

    /**
     **/
    boost::shared_ptr<v3d::font::Font2D> font();

 private:
    boost::shared_ptr<v3d::font::Font2D> font_;
};
};  // namespace v3d::asset::kind
