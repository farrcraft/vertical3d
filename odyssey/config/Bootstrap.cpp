/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Bootstrap.h"

#include <iostream>
#include <string>

#include "../../v3dlibs/asset/JsonFile.h"

#include <boost/filesystem.hpp>

namespace odyssey::config {
    /**
     **/
    Bootstrap::Bootstrap() :
        windowWidth_(300),
        windowHeight_(200) {
    }

    /**
     **/
    bool Bootstrap::load(const boost::shared_ptr<v3d::core::Logger>& logger) {
        try {
            boost::filesystem::path path = boost::filesystem::current_path();
            LOG_INFO(logger) << "Looking for bootstrap.json in: " << path;
            v3d::asset::JsonFile file("bootstrap.json", "r");
            boost::json::stream_parser parser;
            boost::json::error_code err;
            do {
                char buf[4096];
                auto const nread = file.read(buf, sizeof(buf));
                parser.write(buf, nread, err);
            } while (!file.eof());
            if (err) {
                return false;
            }
            parser.finish(err);
            if (err) {
                return false;
            }
            auto const document = parser.release();
            boost::json::object const& object = document.as_object();

            // sanity check that the data path actually exists
            boost::filesystem::path dataPath = boost::filesystem::canonical(boost::filesystem::path(boost::json::value_to<std::string>(object.at("data_path"))));

            if (boost::filesystem::is_directory(dataPath)) {
                dataPath_ = dataPath.string();
                LOG_INFO(logger) << "Found data path: " << dataPath_;
            } else {
                LOG_ERROR(logger) << "Couldn't resolve data path: " << dataPath;
                return false;
            }

            // default window options
            auto const window = object.at("window");
            windowWidth_ = boost::json::value_to<int>(window.at("width"));
            windowHeight_ = boost::json::value_to<int>(window.at("height"));
            LOG_INFO(logger) << "Setting default window size to " << windowWidth_ << "x" << windowHeight_;
        }
        catch (std::exception const& e) {
            LOG_ERROR(logger) << "Bootstrap caught exception: " << e.what();
            return false;
        }

        return true;
    }

    /**
     **/
    int Bootstrap::windowWidth() const {
        return windowWidth_;
    }

    /**
     **/
    int Bootstrap::windowHeight() const {
        return windowHeight_;
    }

    /**
     **/
    std::string_view Bootstrap::dataPath() const {
        return dataPath_;
    }
};  // namespace odyssey::config
