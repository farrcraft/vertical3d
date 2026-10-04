/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::asset::kind {

/**
 **/
Model::Model(const std::string& name, Type t, const boost::shared_ptr<v3d::type::Model>& model,
    const std::vector<boost::shared_ptr<v3d::image::Image>>& baseColours) :
    Asset(name, t),
    model_(model),
    baseColours_(baseColours) {
}

/**
 **/
boost::shared_ptr<v3d::type::Model> Model::model() {
    return model_;
}

/**
 **/
boost::shared_ptr<v3d::image::Image> Model::baseColourImage(std::size_t material) const {
    if (material >= baseColours_.size()) {
        return boost::shared_ptr<v3d::image::Image>();
    }
    return baseColours_[material];
}

};  // namespace v3d::asset::kind
