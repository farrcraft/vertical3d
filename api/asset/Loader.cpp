/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Loader.h"

namespace v3d::asset {
    /**
     **/
    Loader::Loader(Type t, const boost::shared_ptr<v3d::log::Logger>& logger) :
        type_(t),
        logger_(logger) {
    }

    /**
     **/
    Type Loader::type() const {
        return type_;
    }

};  // namespace v3d::asset
