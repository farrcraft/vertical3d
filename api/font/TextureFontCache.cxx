/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFontCache.h"
#include "../image/TextureAtlas.h"
#include "TextureFont.h"

namespace v3d::font {

    wchar_t* wcsdupstr(const wchar_t* string) {
        wchar_t* result;
        size_t len = (wcslen(string) + 1) * sizeof(wchar_t);
        result = new wchar_t[len];
        wcscpy(result, string);
        return result;
    }

    TextureFontCache::TextureFontCache(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
        atlas_.reset(new v3d::image::TextureAtlas(width, height, depth, logger));
        cache_ = wcsdupstr(L" ");
    }

    TextureFontCache::~TextureFontCache() {
        delete[] cache_;
    }

    boost::shared_ptr<v3d::image::TextureAtlas> TextureFontCache::atlas() {
        return atlas_;
    }

    void TextureFontCache::charcodes(const wchar_t* charcodes) {
        delete[] cache_;
        cache_ = wcsdupstr(charcodes);
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
    void TextureFontCache::add(boost::shared_ptr<TextureFont> font) {
        fonts_.push_back(font);
    }

    bool TextureFontCache::remove(boost::shared_ptr<TextureFont> font) {
        for (unsigned int i = 0; i < fonts_.size(); ++i) {
            if (fonts_[i]->filename() == font->filename() && fonts_[i]->size() == font->size()) {
                fonts_.erase(fonts_.begin() + i);
                return true;
            }
        }
        return false;
    }

};  // namespace v3d::font
