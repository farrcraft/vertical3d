/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Json.h"

#include <iostream>
#include <string>

#include "../Json.h"
#include "../Type.h"
#include "../JsonFile.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>

namespace v3d::asset {
    /**
     **/
    loader::Json::Json(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::JsonDocument, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> loader::Json::load(std::string_view name) {
        boost::shared_ptr<v3d::asset::Json> asset;
        try {
            LOG_INFO(logger_) << "Looking for json asset at: " << name;
            JsonFile file(static_cast<std::string>(name).c_str(), "r");
            boost::json::stream_parser parser;
            boost::json::error_code err;
            do {
                char buf[4096];
                auto const nread = file.read(buf, sizeof(buf));
                parser.write(buf, nread, err);
            } while (!file.eof());
            if (err) {
                return asset;
            }
            parser.finish(err);
            if (err) {
                return asset;
            }
            auto const document = parser.release();
            //  boost::json::object const& object = document.as_object();

            asset = boost::make_shared<v3d::asset::Json>(std::string(name), Type::JsonDocument, document.as_object());
        }
        catch (std::exception const& e) {
            LOG_ERROR(logger_) << "Bootstrap caught exception: " << e.what();
        }

        return asset;
    }

};  // namespace v3d::asset
