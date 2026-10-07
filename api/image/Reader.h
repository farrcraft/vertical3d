/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include "Image.h"

#include <cstddef>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::image {
/**
 * One image format, decoded.
 *
 * A format implements the buffer overload, and the path overload is written in terms of it,
 * because an image is not always a file. One embedded in a .glb arrives as a span of the
 * model's own buffer and has no name to open.
 *
 * As a result a file is read whole into memory before it is decoded. The cost is small:
 * these are textures loaded at start up, and the decoded image each reader allocates is
 * larger than the encoded one.
 **/
class Reader {
 public:
        /**
         **/
        explicit Reader(const boost::shared_ptr<v3d::log::Logger> & logger);
        virtual ~Reader() = default;

        /**
         * Read an encoded image out of a file.
         *
         * @return the image, or null when the file cannot be opened or does not decode
         **/
        boost::shared_ptr<Image> read(std::string_view filename);

        /**
         * Read an encoded image out of memory.
         *
         * The buffer has only to outlive the call - what comes back owns its own pixels.
         *
         * @param data the encoded bytes, which are not modified
         * @param size how many of them there are
         * @return the image, or null when the bytes do not decode as this format
         **/
        virtual boost::shared_ptr<Image> read(const unsigned char* data, std::size_t size);

 protected:
     boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::image
