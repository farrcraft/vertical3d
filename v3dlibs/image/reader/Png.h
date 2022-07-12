/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Reader.h"

namespace v3d::image::reader {
    /**
     **/
    class Png : public v3d::image::Reader {
     public:
        explicit Png(const boost::shared_ptr<v3d::core::Logger> & logger);
        ~Png() = default;

        virtual boost::shared_ptr<Image> read(std::string_view filename);
    };

};  // namespace v3d::image::reader
