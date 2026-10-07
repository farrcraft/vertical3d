/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>

namespace v3d::asset::media::loader {
/**
 * Every image format, through image::Factory, which reads the format from the extension.
 *
 * One instance is registered per asset type, so that a caller naming ImagePng gets back an
 * asset that says it was one; the factory decodes the file either way.
 **/
class Image final : public Loader {
 public:
    /**
     * @param type the image type this instance is registered under
     **/
    Image(Type type, const boost::shared_ptr<v3d::log::Logger>& logger);

    boost::shared_ptr<Asset> load(std::string_view name) override;
};
};  // namespace v3d::asset::media::loader
