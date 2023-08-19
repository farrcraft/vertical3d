/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loader.h"

namespace v3d::asset {
/**
    **/
Loader::Loader(Manager& manager, Type t, const boost::shared_ptr<v3d::log::Logger>& logger) :
    manager_(manager),
    type_(t),
    logger_(logger) {
}

void Loader::reset() {

}

void Loader::parameter(const std::string& name, const ParameterValue& value) {
    parameters_[name] = value;
}

boost::optional<ParameterValue> Loader::parameter(std::string_view name) {
    std::map<std::string, ParameterValue>::iterator it = parameters_.find(std::string(name));
    if (it != parameters_.end()) {
        return it->second;
    }
    return boost::none;
}

/**
    **/
Type Loader::type() const {
    return type_;
}

};  // namespace v3d::asset
