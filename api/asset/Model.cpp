/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Model.h"

#include <string>

namespace v3d::asset {

/**
 **/
Model::Model(const std::string& name, Type t, const boost::shared_ptr<v3d::type::Model>& model,
    const boost::shared_ptr<v3d::image::Image>& baseColour) :
    Asset(name, t),
    model_(model),
    baseColour_(baseColour) {
}

/**
 **/
boost::shared_ptr<v3d::type::Model> Model::model() {
    return model_;
}

/**
 **/
boost::shared_ptr<v3d::image::Image> Model::baseColourImage() const {
    return baseColour_;
}

};  // namespace v3d::asset
