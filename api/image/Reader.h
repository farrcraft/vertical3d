/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Image.h"

#include <cstddef>
#include <string>

#include "../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::image {
/**
 * One image format, decoded.
 *
 * The buffer is what a format actually implements and the path is written in terms of it,
 * rather than the other way round: an image is not always a file. One embedded in a .glb
 * arrives as a span of the model's own buffer and has no name to open, and a format whose
 * only entry point took a path could not read it at all.
 *
 * Reading the whole file in first is what that costs, and it is not much of one. These are
 * textures, they are loaded at start up, and the decoded image every reader allocates
 * beside the encoded one is the larger of the two.
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
