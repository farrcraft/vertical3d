/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Json.h"

#include <api/asset/Asset.h>
#include <api/asset/File.h>
#include <api/asset/Loader.h>
#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>

#include <optional>
#include <string>
#include <string_view>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>

namespace v3d::asset {
/**
 **/
loader::Json::Json(const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(typeid(v3d::asset::kind::Json), logger) {
}

/**
 **/
boost::shared_ptr<Asset> loader::Json::load(std::string_view name) {
    logger()->get()->info("Looking for json asset at: {}", name);
    const std::optional<std::string> text = readFile(name);
    if (!text) {
        logger()->get()->error("Could not read json asset: {}", name);
        return boost::shared_ptr<Asset>();
    }
    boost::system::error_code error;
    const boost::json::value document = boost::json::parse(*text, error);
    if (error || !document.is_object()) {
        logger()->get()->error("{} is not a json object: {}", name, error ? error.message() : "it is some other value");
        return boost::shared_ptr<Asset>();
    }
    return boost::make_shared<v3d::asset::kind::Json>(std::string(name), document.as_object());
}

};  // namespace v3d::asset
