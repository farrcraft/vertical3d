/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/asset/Asset.h>
#include <api/font/TextureFont.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::asset::kind {
/**
 **/
class TextureFont : public Asset {
 public:
    /**
     **/
    TextureFont(const std::string& name, Type t, const boost::shared_ptr<v3d::font::TextureFont>& font);

    /**
     **/
    boost::shared_ptr<v3d::font::TextureFont> font();

 private:
    boost::shared_ptr<v3d::font::TextureFont> font_;
};
};  // namespace v3d::asset::kind
