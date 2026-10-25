/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Writer.h"

#include <api/log/Logger.h>

namespace v3d::image {
/**
 **/
Writer::Writer(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
}

const boost::shared_ptr<v3d::log::Logger>& Writer::logger() const noexcept {
    return logger_;
}

};  // namespace v3d::image
