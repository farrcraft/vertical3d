/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Loader.h"

namespace odyssey::asset {

    /**
     **/
    Loader::Loader(Type t, const boost::shared_ptr<v3d::core::Logger>& logger) :
        type_(t),
        logger_(logger) {

    }

    /**
     **/
    Type Loader::type() const {
        return type_;
    }

};  // namespace odyssey::asset
