/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <OpenColorIO/OpenColorIO.h>

#include <utility>

namespace v3d::moya::colour::internal {

/** An OpenColorIO configuration, behind colour::Config. **/
class Configuration final {
 public:
    explicit Configuration(OCIO_NAMESPACE::ConstConfigRcPtr config) : config_(std::move(config)) {
    }

    const OCIO_NAMESPACE::ConstConfigRcPtr & get() const {
        return config_;
    }

 private:
    OCIO_NAMESPACE::ConstConfigRcPtr config_;
};

};  // namespace v3d::moya::colour::internal
