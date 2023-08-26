/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Asset.h"
#include "../font/Font2D.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
    /**
     **/
    class Font2D : public Asset {
    public:
        /**
         **/
        Font2D(const std::string& name, Type t, boost::shared_ptr<v3d::font::Font2D> font);

        /**
         **/
        boost::shared_ptr<v3d::font::Font2D> font();

    private:
        boost::shared_ptr<v3d::font::Font2D> font_;
    };
};  // namespace v3d::asset
