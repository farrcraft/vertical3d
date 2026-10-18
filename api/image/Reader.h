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
 * A format's reader implements the buffer overload of read(). The path overload reads the
 * whole file into memory and passes it to the buffer overload. An image is not always a file:
 * one embedded in a .glb arrives as a span of the model's buffer, with no name to open.
 *
 * Reading a file whole costs little, because the decoded image a reader allocates is larger
 * than the encoded file.
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
    /**
     * @return the log a format reports a failure to
     **/
    const boost::shared_ptr<v3d::log::Logger>& logger() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::image
