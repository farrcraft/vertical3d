/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TextureFont.h"

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::font {
    class TextureFontCache {
     public:
        TextureFontCache(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::core::Logger> & logger);
        ~TextureFontCache();

        void charcodes(const wchar_t * charcodes);

        boost::shared_ptr<TextureFont> load(const std::string & filename, float size);
        bool remove(boost::shared_ptr<TextureFont> font);

        boost::shared_ptr<v3d::image::TextureAtlas> atlas();

     private:
        boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
        std::vector<boost::shared_ptr<TextureFont> > fonts_;
        wchar_t * cache_;
        boost::shared_ptr<v3d::core::Logger> logger_;
    };
};  // namespace v3d::font
