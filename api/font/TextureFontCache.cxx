/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFontCache.h"

#include <api/image/TextureAtlas.h>

#include <string>
#include "TextureFont.h"

namespace v3d::font {

TextureFontCache::TextureFontCache(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
    atlas_.reset(new v3d::image::TextureAtlas(width, height, depth, logger));
}

boost::shared_ptr<v3d::image::TextureAtlas> TextureFontCache::atlas() {
    return atlas_;
}

/**
 **/
boost::shared_ptr<TextureFont> TextureFontCache::find(const std::string& filename, float size) {
    for (unsigned int i = 0; i < fonts_.size(); ++i) {
        if (fonts_[i]->filename() == filename && fonts_[i]->size() == size) {
            return fonts_[i];
        }
    }

    return nullptr;
}

/**
 **/
void TextureFontCache::add(const boost::shared_ptr<TextureFont>& font) {
    fonts_.push_back(font);
}

bool TextureFontCache::remove(const boost::shared_ptr<TextureFont>& font) {
    for (unsigned int i = 0; i < fonts_.size(); ++i) {
        if (fonts_[i]->filename() == font->filename() && fonts_[i]->size() == font->size()) {
            fonts_.erase(fonts_.begin() + i);
            return true;
        }
    }
    return false;
}

};  // namespace v3d::font
