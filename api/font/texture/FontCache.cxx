/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "FontCache.h"

#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include <string>
#include "Font.h"

namespace v3d::font::texture {

FontCache::FontCache(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
    atlas_.reset(new v3d::image::TextureAtlas(width, height, depth, logger));
}

boost::shared_ptr<v3d::image::TextureAtlas> FontCache::atlas() {
    return atlas_;
}

/**
 **/
boost::shared_ptr<Font> FontCache::find(const std::string& filename, float size) {
    for (unsigned int i = 0; i < fonts_.size(); ++i) {
        if (fonts_[i]->filename() == filename && fonts_[i]->size() == size) {
            return fonts_[i];
        }
    }

    return nullptr;
}

/**
 **/
void FontCache::add(const boost::shared_ptr<Font>& font) {
    fonts_.push_back(font);
}

bool FontCache::remove(const boost::shared_ptr<Font>& font) {
    for (unsigned int i = 0; i < fonts_.size(); ++i) {
        if (fonts_[i]->filename() == font->filename() && fonts_[i]->size() == font->size()) {
            fonts_.erase(fonts_.begin() + i);
            return true;
        }
    }
    return false;
}

};  // namespace v3d::font::texture
