/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/asset/Loader.h>

namespace v3d::asset::loader {
/**
 * Reads a file as a JSON object and gives a kind::Json. A file that cannot be read, or that
 * holds anything but an object, gives no asset.
 **/
class Json final : public Loader {
 public:
    /**
     **/
    Json(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     **/
    boost::shared_ptr<Asset> load(std::string_view name);
};
};  // namespace v3d::asset::loader
