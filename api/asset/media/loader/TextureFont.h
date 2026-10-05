/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>

namespace v3d::asset::media::loader {
/**
 **/
class TextureFont final : public Loader {
 public:
    /**
     **/
    TextureFont(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     **/
    boost::shared_ptr<Asset> load(std::string_view name);
};
};  // namespace v3d::asset::media::loader
