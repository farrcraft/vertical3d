/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "BindingContext.h"

using namespace odyssey::config;

/**
 **/
BindingContext::BindingContext(const std::string_view &context) :
	context_(context) {
}

/**
 **/
bool BindingContext::setBinding(const std::string_view& key, const std::string_view& binding) {
	bool exists = bindings_.contains(binding);
	bindings_[key] = binding;
	return exists;
}
