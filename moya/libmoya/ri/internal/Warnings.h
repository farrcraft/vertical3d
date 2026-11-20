/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>

#include <set>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::ri::internal {

/** Logs a warning once for each key, however often the same case arises. **/
class Warnings final {
 public:
    explicit Warnings(const boost::shared_ptr<v3d::log::Logger> & logger);

    /** Log a message, unless one was already logged under the same key. **/
    void once(const std::string & key, const std::string & message);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::set<std::string> logged_;
};

};  // namespace v3d::moya::ri::internal
