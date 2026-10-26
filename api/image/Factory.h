/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Reader.h"
#include "Writer.h"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>

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
     * A buffer has no file name to take an extension from, so the caller names the format.
     * An embedded image's mime type or its container tells the caller which format it is.
     *
     * @param kind the format's key, as the readers were registered under: "png", "jpg",
     *        "jpeg", "bmp", "tga"
     * @return the image, or null when nothing reads that format or the bytes do not decode
     **/
    boost::shared_ptr<Image> read(const unsigned char* data, std::size_t size, std::string_view kind);

    /**
     * Encode an image in a format, for the caller to write.
     *
     * Nothing here writes a file. A caller writes the bytes through asset::writeFile, which
     * replaces a file atomically.
     *
     * @param kind the format's key, as kind() gives it for a file name
     * @return the encoded file, or nothing when no writer has that key or the image does not
     *         encode
     **/
    std::optional<std::string> encode(const boost::shared_ptr<Image> & img, std::string_view kind);

    /**
     * The key a file's format is registered under: its extension, lower case and without the
     * dot. A name with no extension gives an empty key, which nothing is under.
     **/
    static std::string kind(std::string_view filename);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::map<std::string, boost::shared_ptr<Reader> > readers_;
    std::map<std::string, boost::shared_ptr<Writer> > writers_;
};

};  // namespace v3d::image
