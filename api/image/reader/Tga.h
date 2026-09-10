/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Reader.h>

#include <cstddef>

namespace v3d::image::reader {
/**
 **/
class Tga : public v3d::image::Reader {
 public:
    explicit Tga(const boost::shared_ptr<v3d::log::Logger> & logger);
    ~Tga() = default;

    // the path form is the base's, written in terms of this one
    using v3d::image::Reader::read;

    boost::shared_ptr<Image> read(const unsigned char* data, std::size_t size) override;
};

};  // namespace v3d::image::reader
