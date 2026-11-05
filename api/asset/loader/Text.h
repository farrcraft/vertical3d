/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>

namespace v3d::asset::loader {
/**
 * Reads a whole file as text and gives a kind::Text. A file that cannot be read gives no asset.
 **/
class Text final : public Loader {
 public:
    /**
     **/
    Text(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     **/
    boost::shared_ptr<Asset> load(std::string_view name);
};
};  // namespace v3d::asset::loader
