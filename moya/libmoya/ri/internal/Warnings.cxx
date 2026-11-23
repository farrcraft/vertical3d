/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Warnings.h"

#include <api/log/Logger.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::ri::internal {

Warnings::Warnings(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the key, then what is logged
void Warnings::once(const std::string & key, const std::string & message) {
    if (logged_.insert(key).second) {
        logger_->get()->warn("{}", message);
    }
}

};  // namespace v3d::moya::ri::internal
