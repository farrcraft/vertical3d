/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loader.h"

#include <api/log/Logger.h>

#include "Type.h"

namespace v3d::asset {
/**
    **/
Loader::Loader(Type t, const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    type_(t) {
}

/**
    **/
Type Loader::type() const {
    return type_;
}

const boost::shared_ptr<v3d::log::Logger>& Loader::logger() const noexcept {
    return logger_;
}

};  // namespace v3d::asset
