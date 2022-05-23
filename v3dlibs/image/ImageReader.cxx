/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ImageReader.h"

using namespace v3d::image;

ImageReader::ImageReader() {
}

ImageReader::~ImageReader() {
}

boost::shared_ptr<Image> ImageReader::read(const std::string & filename) {
	boost::shared_ptr<Image> empty_ptr;
	return empty_ptr;
}
