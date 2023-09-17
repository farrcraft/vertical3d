/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include "Asset.h"
#include "../font/TextureFont.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
    /**
     **/
    class TextureFont : public Asset {
     public:
        /**
         **/
        TextureFont(const std::string& name, Type t, boost::shared_ptr<v3d::font::TextureFont> font);

        /**
         **/
        boost::shared_ptr<v3d::font::TextureFont> font();

     private:
        boost::shared_ptr<v3d::font::TextureFont> font_;
    };
};  // namespace v3d::asset
