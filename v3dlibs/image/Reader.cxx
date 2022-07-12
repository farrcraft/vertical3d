/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Reader.h"

namespace v3d::image {
    /**
     **/
    Reader::Reader(const boost::shared_ptr<v3d::core::Logger>& logger) : logger_(logger) {
    }

    /**
     **/
    boost::shared_ptr<Image> Reader::read(std::string_view filename) {
        boost::shared_ptr<Image> empty_ptr;
        return empty_ptr;
    }

};  // namespace v3d::image
