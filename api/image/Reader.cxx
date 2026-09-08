/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Reader.h"

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace v3d::image {
/**
 **/
Reader::Reader(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
}

/**
 **/
boost::shared_ptr<Image> Reader::read(std::string_view filename) {
    boost::shared_ptr<Image> empty_ptr;

    std::ifstream file(static_cast<std::string>(filename).c_str(), std::ifstream::in | std::ifstream::binary);
    if (!file) {
        logger_->get()->error("the image {} could not be opened", filename);
        return empty_ptr;
    }
    file.seekg(0, std::ifstream::end);
    const std::streamoff length = file.tellg();
    file.seekg(0, std::ifstream::beg);
    if (length <= 0) {
        logger_->get()->error("the image {} holds nothing", filename);
        return empty_ptr;
    }

    std::vector<unsigned char> encoded(static_cast<std::size_t>(length));
    file.read(reinterpret_cast<char*>(encoded.data()), length);
    if (!file) {
        logger_->get()->error("the image {} could not be read to the end", filename);
        return empty_ptr;
    }

    return read(encoded.data(), encoded.size());
}

/**
 **/
boost::shared_ptr<Image> Reader::read(const unsigned char* /* data */, std::size_t /* size */) {
    boost::shared_ptr<Image> empty_ptr;
    return empty_ptr;
}

};  // namespace v3d::image
