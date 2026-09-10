/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Manager.h"

#include <api/asset/loader/Font2D.h>
#include <api/asset/loader/Gltf.h>
#include <api/asset/loader/Jpeg.h>
#include <api/asset/loader/Json.h>
#include <api/asset/loader/Png.h>
#include <api/asset/loader/Text.h>
#include <api/asset/loader/TextureFont.h>
#include <api/asset/loader/Tga.h>
#include <api/asset/loader/Wav.h>

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::asset {

/**
 **/
Manager::Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
    path_ = static_cast<std::string>(path);
    logger_->get()->info("Setting asset manager path to: {}", path);
    loaders_[asset::Type::ImageJpeg] = boost::make_shared<v3d::asset::loader::Jpeg>(this, logger_);
    loaders_[asset::Type::ImagePng] = boost::make_shared<v3d::asset::loader::Png>(this, logger_);
    loaders_[asset::Type::ImageTga] = boost::make_shared<v3d::asset::loader::Tga>(this, logger_);
    loaders_[asset::Type::ModelGltf] = boost::make_shared<v3d::asset::loader::Gltf>(this, logger_);
    loaders_[asset::Type::JsonDocument] = boost::make_shared<v3d::asset::loader::Json>(this, logger_);
    loaders_[asset::Type::AudioWav] = boost::make_shared<v3d::asset::loader::Wav>(this, logger_);
    loaders_[asset::Type::Text] = boost::make_shared<v3d::asset::loader::Text>(this, logger_);
    loaders_[asset::Type::Font2D] = boost::make_shared<v3d::asset::loader::Font2D>(this, logger_);
    loaders_[asset::Type::TextureFont] = boost::make_shared<v3d::asset::loader::TextureFont>(this, logger_);
}

/**
 **/
boost::shared_ptr<Loader> Manager::resolveLoader(asset::Type t) {
    auto search = loaders_.find(t);
    if (search == loaders_.end()) {
        throw std::invalid_argument("no loader for type");
    }
    return search->second;
}

/**
 **/
boost::shared_ptr<Asset> Manager::load(std::string_view name, asset::Type t, bool hasPath) {
    boost::filesystem::path assetPath;
    // recursive asset loaders will already have a path set
    if (!hasPath) {
        assetPath = path_;
    }
    assetPath /= static_cast<std::string>(name);

    boost::shared_ptr<Loader> loader = resolveLoader(t);
    boost::shared_ptr<Asset> asset = loader->load(assetPath.string());
    return asset;
}

/**
 **/
boost::shared_ptr<Asset> Manager::loadTypeFromExt(std::string_view name) {
    boost::filesystem::path pathName(static_cast<std::string>(name));
    boost::shared_ptr<Asset> asset;
    std::string ext = pathName.extension().string();
    if (ext == ".png") {
        asset = load(name, asset::Type::ImagePng);
    } else if (ext == ".jpg") {
        asset = load(name, asset::Type::ImageJpeg);
    } else if (ext == ".tga") {
        asset = load(name, asset::Type::ImageTga);
    } else if (ext == ".gltf" || ext == ".glb") {
        asset = load(name, asset::Type::ModelGltf);
    } else if (ext == ".json") {
        asset = load(name, asset::Type::JsonDocument);
    } else if (ext == ".wav") {
        asset = load(name, asset::Type::AudioWav);
    } else {
        throw std::invalid_argument("unrecognized extension");
    }
    return asset;
}

};  // namespace v3d::asset
