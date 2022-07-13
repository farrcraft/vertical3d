/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Asset.h"

using namespace odyssey::asset;

Asset::Asset(std::string_view name, asset::Type t) :
	name_(name),
	type_(t) {

}
