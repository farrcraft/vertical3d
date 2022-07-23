/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Asset.h"

#include <vector>

namespace v3d::asset {
    /**
     **/
    class Cache {
     public:
        /**
         **/
        explicit Cache(Type t);

     private:
        std::vector<Asset> assets_;
        Type type_;
    };
};  // namespace v3d::asset
