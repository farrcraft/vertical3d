/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Factory.h"

#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

#include "reader/Tga.h"
#include "reader/Bmp.h"
#include "reader/Png.h"
#include "reader/Jpeg.h"

#include "writer/Tga.h"
#include "writer/Bmp.h"
#include "writer/Png.h"
#include "writer/Jpeg.h"

namespace v3d::image {
    /**
     **/
    Factory::Factory(const boost::shared_ptr<v3d::core::Logger>& logger) : logger_(logger) {
        add("tga", boost::make_shared<reader::Tga>(logger));
        add("bmp", boost::make_shared<reader::Bmp>(logger));
        add("png", boost::make_shared<reader::Png>(logger));
        add("jpg", boost::make_shared<reader::Jpeg>(logger));

        add("tga", boost::make_shared<writer::Tga>(logger));
        add("bmp", boost::make_shared<writer::Bmp>(logger));
        add("png", boost::make_shared<writer::Png>(logger));
        add("jpg", boost::make_shared<writer::Jpeg>(logger));
    }

    void Factory::add(const std::string& name, const boost::shared_ptr<Reader>& reader) {
        readers_[name] = reader;
    }

    void Factory::add(const std::string& name, const boost::shared_ptr<Writer>& writer) {
        writers_[name] = writer;
    }

    bool Factory::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
        boost::filesystem::path full_path = boost::filesystem::system_complete(static_cast<std::string>(filename));

        std::string ext = static_cast<std::string>(filename).substr(filename.length() - 3);
        std::map<std::string, boost::shared_ptr<Writer> >::iterator it = writers_.find(ext);
        if (it != writers_.end()) {
            boost::shared_ptr<Writer> writer = (*it).second;
            return writer->write(full_path.string(), img);
        }
        return false;
    }

    boost::shared_ptr<Image> Factory::read(std::string_view filename) {
        boost::filesystem::path full_path = boost::filesystem::system_complete(static_cast<std::string>(filename));
        std::string filepath = full_path.string();

        std::string ext = static_cast<std::string>(filename).substr(filename.length() - 3);

        LOG_DEBUG(logger_) << "ImageFactory::read - reading file [" << filename << "] with reader bound to extension [" << ext << "] from path [" << filepath << "]";

        std::map<std::string, boost::shared_ptr<Reader> >::iterator it = readers_.find(ext);
        if (it != readers_.end()) {
            boost::shared_ptr<Reader> reader = (*it).second;
            return reader->read(filepath);
        }
        boost::shared_ptr<Image> empty_ptr;
        LOG_DEBUG(logger_) << "ImageFactory::read - no reader exists for detected image format!";
        return empty_ptr;
    }

};  // namespace v3d::image
