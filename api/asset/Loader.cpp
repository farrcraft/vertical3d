/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loader.h"

#include <api/log/Logger.h>

#include <typeindex>
#include <typeinfo>

namespace v3d::asset {
/**
    **/
Loader::Loader(const std::type_info& kind, const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    kind_(kind) {
}

/**
    **/
std::type_index Loader::kind() const noexcept {
    return kind_;
}

const boost::shared_ptr<v3d::log::Logger>& Loader::logger() const noexcept {
    return logger_;
}

};  // namespace v3d::asset
