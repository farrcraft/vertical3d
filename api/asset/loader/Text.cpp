/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Text.h"

#include <api/asset/Text.h>
#include <api/asset/Type.h>

#include <fstream>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

/**
 **/
Text::Text(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::Text, logger) {
}

/**
 **/
boost::shared_ptr<Asset> Text::load(std::string_view name) {
    // read shader file content
    std::ifstream file(std::string(name).c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        char reason[256] = {};
        strerror_s(reason, sizeof(reason), errno);
        std::string err = std::string("error loading asset file: ") + std::string(name) + std::string(" - ") + reason;
        throw std::runtime_error(err);
    }
    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(static_cast<unsigned int>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(content.data(), content.size());
    file.close();
    boost::shared_ptr<Asset> text = boost::make_shared<v3d::asset::Text>(std::string(name), Type::Text, content);

    return text;
}
};  // namespace v3d::asset::loader
