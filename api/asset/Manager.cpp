/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Manager.h"

#include <string>

#include "loader/Jpeg.h"
#include "loader/Json.h"
#include "loader/Png.h"

#include <boost/make_shared.hpp>

namespace v3d::asset {

    /**
     **/
    Manager::Manager(std::string_view path, const boost::shared_ptr<v3d::log::Logger>& logger) :
        logger_(logger) {
        path_ = static_cast<std::string>(path);
        LOG_INFO(logger_) << "Setting asset manager path to: " << path_.c_str();
        loaders_[asset::Type::IMAGE_JPEG] = boost::make_shared<v3d::asset::loader::Jpeg>(logger_);
        loaders_[asset::Type::IMAGE_PNG] = boost::make_shared<v3d::asset::loader::Png>(logger_);
        loaders_[asset::Type::JSON_DOCUMENT] = boost::make_shared<v3d::asset::loader::Json>(logger_);
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
    boost::shared_ptr<Asset> Manager::load(std::string_view name, asset::Type t) {
        boost::filesystem::path assetPath = path_;
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
            asset = load(name, asset::Type::IMAGE_PNG);
        } else if (ext == ".jpg") {
            asset = load(name, asset::Type::IMAGE_JPEG);
        } else if (ext == ".json") {
            asset = load(name, asset::Type::JSON_DOCUMENT);
        } else {
            throw std::invalid_argument("unrecognized extension");
        }
        return asset;
    }

};  // namespace v3d::asset
