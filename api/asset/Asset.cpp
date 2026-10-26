/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Asset.h"

#include <string>

namespace v3d::asset {

Asset::Asset(const std::string& name) :
    name_(name) {
}

const std::string& Asset::name() const noexcept {
    return name_;
}

};  // namespace v3d::asset
