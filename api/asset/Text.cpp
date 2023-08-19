/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Text.h"

namespace v3d::asset {

    /**
     **/
    Text::Text(const std::string& name, Type t, const std::string& content) :
        Asset(name, t),
        content_(content) {
    }

    /**
     **/
    std::string const& Text::content() {
        return content_;
    }

};  // namespace v3d::asset
