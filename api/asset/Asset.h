/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Type.h"

#include <string>

namespace v3d::asset {
/**
 **/
class Asset {
 public:
    /**
     **/
    Asset(const std::string& name, Type t);

    /**
     * Default destructor
     **/
    virtual ~Asset() = default;

    /**
     * @return where the asset was loaded from
     **/
    const std::string& name() const noexcept;

    /**
     * @return the type it was loaded as
     **/
    Type type() const noexcept;

 protected:
    std::string name_;
    Type type_;
};
};  // namespace v3d::asset
