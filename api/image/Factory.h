/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Reader.h"
#include "Writer.h"

#include <cstddef>
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

    /**
     * Decode an image that arrived as bytes rather than as a file.
     *
     * A buffer carries no name, so the format has to be said rather than worked out from
     * an extension - which is what an embedded image's mime type or its container says.
     *
     * @param kind the format's key, as the readers were registered under: "png", "jpg",
     *        "bmp", "tga"
     * @return the image, or null when nothing reads that format or the bytes do not decode
     **/
    boost::shared_ptr<Image> read(const unsigned char* data, std::size_t size, std::string_view kind);

    bool write(std::string_view filename, const boost::shared_ptr<Image> & img);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::map<std::string, boost::shared_ptr<Reader> > readers_;
    std::map<std::string, boost::shared_ptr<Writer> > writers_;
};

};  // namespace v3d::image
