/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Loader.h"

using namespace odyssey::asset;

/**
 **/
Loader::Loader(Type t, const boost::shared_ptr<odyssey::engine::Logger> & logger) :
	type_(t),
	logger_(logger) {

}

/**
 **/
Type Loader::type() const {
	return type_;
}
