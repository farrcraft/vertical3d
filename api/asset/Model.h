/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Asset.h"
#include "../type/Model.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
/**
 * Geometry loaded from a model file, as the asset manager hands it out.
 **/
class Model : public Asset {
 public:
    /**
     **/
    Model(const std::string& name, Type t, const boost::shared_ptr<v3d::type::Model>& model);

    /**
     **/
    boost::shared_ptr<v3d::type::Model> model();

 private:
    boost::shared_ptr<v3d::type::Model> model_;
};
};  // namespace v3d::asset
