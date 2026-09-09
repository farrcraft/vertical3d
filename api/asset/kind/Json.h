/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/asset/Asset.h>

#include <string>

#include <boost/json.hpp>

namespace v3d::asset::kind {
/**
 * A JSON document asset
 **/
class Json final : public Asset {
 public:
    /**
     **/
    Json(const std::string& name, Type t, const boost::json::object &doc);

    /**
     **/
    boost::json::object const& document();

 private:
    boost::json::object document_;
};
};  // namespace v3d::asset::kind
