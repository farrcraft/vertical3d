/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Model.h"

#include <string>

namespace v3d::asset {

/**
 **/
Model::Model(const std::string& name, Type t, const boost::shared_ptr<v3d::type::Model>& model) :
    Asset(name, t),
    model_(model) {
}

/**
 **/
boost::shared_ptr<v3d::type::Model> Model::model() {
    return model_;
}

};  // namespace v3d::asset
