/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Type.h"

#include <string_view>

namespace v3d::asset {
    /**
     **/
    class Asset {
     public:
        /**
         **/
        Asset(std::string name, Type t);

        /**
         * Default destructor
         **/
        virtual ~Asset() = default;

     protected:
        std::string name_;
        Type type_;
    };
};  // namespace v3d::asset
