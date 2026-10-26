/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>

namespace v3d::asset::media::loader {
/**
 * Every image format, through image::Factory, which reads the format from the extension.
 * Every format produces the one kind, kind::Image.
 **/
class Image final : public Loader {
 public:
    explicit Image(const boost::shared_ptr<v3d::log::Logger>& logger);

    boost::shared_ptr<Asset> load(std::string_view name) override;
};
};  // namespace v3d::asset::media::loader
