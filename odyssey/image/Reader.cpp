/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Reader.h"

using namespace odyssey::image;

/**
 **/
Reader::Reader() {
}

/**
 **/
Reader::~Reader() {
}

/**
 **/
boost::shared_ptr<Image> Reader::read(std::string_view filename) {
	boost::shared_ptr<Image> empty_ptr;
	return empty_ptr;
}
