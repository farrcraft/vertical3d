/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Asset.h"

#include <boost/json.hpp>

namespace v3d::asset {
    /**
     * A simple text asset
     **/
    class Text final : public Asset {
    public:
        /**
         **/
        Text(const std::string& name, Type t, const std::string& content);

        /**
         **/
        std::string const& content();

    private:
        std::string content_;
    };
};  // namespace v3d::asset
