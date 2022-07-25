/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Image.h"

#include <string>

#include "../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::image {
    /**
     *  
     **/
    class Reader {
     public:
            /**
             **/
            explicit Reader(const boost::shared_ptr<v3d::log::Logger> & logger);
            virtual ~Reader() = default;

            /**
             **/
            virtual boost::shared_ptr<Image> read(std::string_view filename);

     protected:
         boost::shared_ptr<v3d::log::Logger> logger_;
    };

};  // namespace v3d::image
