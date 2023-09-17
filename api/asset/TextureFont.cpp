/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "TextureFont.h"

namespace v3d::asset {
    /**
     **/
    TextureFont::TextureFont(const std::string& name, Type t, boost::shared_ptr<v3d::font::TextureFont> font) :
        Asset(name, t),
        font_(font) {
    }

    /**
     **/
    boost::shared_ptr<v3d::font::TextureFont> TextureFont::font() {
        return font_;
    }

};  // namespace v3d::asset
