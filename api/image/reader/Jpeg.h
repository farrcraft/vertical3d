/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Reader.h"

namespace v3d::image::reader {
    /**
     **/
    class Jpeg final : public v3d::image::Reader {
     public:
        Jpeg(const boost::shared_ptr<v3d::log::Logger> & logger);
        ~Jpeg() = default;

        virtual boost::shared_ptr<Image> read(std::string_view filename);
    };

};  // namespace v3d::image::reader
