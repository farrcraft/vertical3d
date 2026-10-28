/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include "Image.h"

#include <optional>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::image {
/**
 * One image format, encoded.
 *
 * A writer encodes to bytes and writes no file. The caller writes them, through
 * asset::writeFile for a file that has to be replaced atomically.
 **/
class Writer {
 public:
    explicit Writer(const boost::shared_ptr<v3d::log::Logger>& logger);
    virtual ~Writer() = default;

    /**
     * Encode the image in this format. Each format implements it.
     *
     * @return the encoded file, or nothing when the image cannot be encoded in this format
     **/
    virtual std::optional<std::string> encode(const boost::shared_ptr<Image> & img) = 0;
 protected:
    /**
     * @return the log a format reports a failure to
     **/
    const boost::shared_ptr<v3d::log::Logger>& logger() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::image
