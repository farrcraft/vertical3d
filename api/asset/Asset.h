/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

namespace v3d::asset {
/**
 * Something a loader read from a file. Its class is its kind: a loader is registered for the
 * class it produces, and a caller holds the asset as that class.
 **/
class Asset {
 public:
    /**
     **/
    explicit Asset(const std::string& name);

    /**
     * Default destructor
     **/
    virtual ~Asset() = default;

    /**
     * @return where the asset was loaded from
     **/
    const std::string& name() const noexcept;

 private:
    std::string name_;
};
};  // namespace v3d::asset
