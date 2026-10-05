/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Text.h"

#include <api/asset/File.h>
#include <api/asset/Type.h>
#include <api/asset/kind/Text.h>

#include <optional>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

/**
 **/
Text::Text(const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(Type::Text, logger) {
}

/**
 **/
boost::shared_ptr<Asset> Text::load(std::string_view name) {
    const std::optional<std::string> content = readFile(name);
    if (!content) {
        logger_->get()->error("Could not read text asset: {}", name);
        return boost::shared_ptr<Asset>();
    }
    return boost::make_shared<v3d::asset::kind::Text>(std::string(name), Type::Text, *content);
}
};  // namespace v3d::asset::loader
