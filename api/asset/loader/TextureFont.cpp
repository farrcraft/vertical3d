/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFont.h"

#include <api/asset/TextureFont.h>
#include <api/asset/Type.h>
#include <api/font/TextureFont.h>

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

/**
 **/
TextureFont::TextureFont(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::TextureFont, logger) {
}

/**
 **/
boost::shared_ptr<Asset> TextureFont::load(std::string_view name) {
    boost::optional<ParameterValue> param = parameter("fontSize");
    if (!param) {
        return nullptr;
    }
    float fontSize = std::get<float>(param.get());

    // a face asked for without one is rasterized as coverage, which is what everything
    // that is not drawing a scalable glyph wants
    unsigned int spread = 0;
    boost::optional<ParameterValue> field = parameter("spread");
    if (field) {
        spread = static_cast<unsigned int>(std::get<float>(field.get()));
    }

    boost::shared_ptr<v3d::font::TextureFont> font = boost::make_shared<v3d::font::TextureFont>(std::string(name), fontSize, logger_, spread);
    boost::shared_ptr<Asset> asset = boost::make_shared<v3d::asset::TextureFont>(std::string(name), type(), font);
    return asset;
}

};  // namespace v3d::asset::loader
