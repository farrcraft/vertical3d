/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Image.h"

#include <string>

#include <boost/shared_ptr.hpp>

#include "../log/Logger.h"

namespace v3d::image {
    /**
     *  
     **/
    class Writer {
     public:
        explicit Writer(const boost::shared_ptr<v3d::log::Logger>& logger);
        virtual ~Writer() = default;

        virtual bool write(std::string_view filename, const boost::shared_ptr<Image> & img);
     protected:
         boost::shared_ptr<v3d::log::Logger> logger_;
    };

};  // namespace v3d::image
