/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Writer.h"

namespace v3d::image::writer {
    /** 
     **/
    class Png : public v3d::image::Writer {
     public:
        /**
         **/
        explicit Png(const boost::shared_ptr<v3d::log::Logger> & logger);
        ~Png() = default;

        virtual bool write(std::string_view filename, const boost::shared_ptr<Image> & img);
    };

};  // namespace v3d::image::writer
