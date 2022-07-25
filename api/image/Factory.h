/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Reader.h"
#include "Writer.h"

#include <map>
#include <string>

namespace v3d::image {
    /**
     * 
     **/
    class Factory {
     public:
        explicit Factory(const boost::shared_ptr<v3d::log::Logger> & logger);
        ~Factory() = default;

        void add(const std::string & name, const boost::shared_ptr<Reader> & reader);
        void add(const std::string & name, const boost::shared_ptr<Writer> & writer);

        boost::shared_ptr<Image> read(std::string_view filename);
        bool write(std::string_view filename, const boost::shared_ptr<Image> & img);

     private:
        boost::shared_ptr<v3d::log::Logger> logger_;
        std::map<std::string, boost::shared_ptr<Reader> > readers_;
        std::map<std::string, boost::shared_ptr<Writer> > writers_;
    };

};  // namespace v3d::image
