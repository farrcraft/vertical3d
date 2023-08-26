/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Font2D.h"

namespace v3d::asset {
    /**
     **/
    Font2D::Font2D(const std::string& name, Type t, boost::shared_ptr<v3d::font::Font2D> font) :
        Asset(name, t),
        font_(font) {
    }

    /**
     **/
    boost::shared_ptr<v3d::font::Font2D> Font2D::font() {
        return font_;
    }

};  // namespace v3d::asset
