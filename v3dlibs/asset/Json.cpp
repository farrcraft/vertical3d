/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Json.h"

namespace v3d::asset {

    /**
     **/
    Json::Json(std::string_view name, Type t, const boost::json::object& doc) :
        Asset(name, t),
        document_(doc) {
    }

    /**
     **/
    boost::json::object const& Json::document() {
        return document_;
    }

};  // namespace v3d::asset
