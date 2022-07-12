/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Writer.h"

namespace v3d::image::writer {

    class Bmp final : public v3d::image::Writer {
     public:
        Bmp(const boost::shared_ptr<v3d::core::Logger> & logger);
        ~Bmp() = default;

        virtual bool write(std::string_view filename, const boost::shared_ptr<Image> & img);
    };
};  // namespace v3d::image::writer
