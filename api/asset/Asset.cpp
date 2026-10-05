/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Asset.h"

#include <string>

namespace v3d::asset {

Asset::Asset(const std::string& name, asset::Type t) :
    name_(name),
    type_(t) {
}

const std::string& Asset::name() const noexcept {
    return name_;
}

Type Asset::type() const noexcept {
    return type_;
}

};  // namespace v3d::asset
