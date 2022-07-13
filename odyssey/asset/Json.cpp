/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Json.h"

using namespace odyssey::asset;

/**
 **/
Json::Json(std::string_view name, Type t, const boost::json::object &doc) :
	Asset(name, t),
	document_(doc) {

}

/**
 **/
boost::json::object const& Json::document() {
	return document_;
}
