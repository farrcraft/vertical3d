/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include "Config.h"

#include <boost/make_shared.hpp>

using namespace odyssey::config;

/**
 **/
Config::Config(const boost::shared_ptr<odyssey::engine::Logger>& logger) : logger_(logger) {

}

/**
 **/
bool Config::load(const boost::shared_ptr<odyssey::asset::Manager>& assetManager) {
	boost::shared_ptr<odyssey::asset::Json> bindings = boost::dynamic_pointer_cast<odyssey::asset::Json>(assetManager->loadTypeFromExt("bindings.json"));
	if (!bindings || !loadBindings(bindings)) {
		return false;
	}
	return true;
}

/**
 **/
bool Config::loadBindings(const boost::shared_ptr<odyssey::asset::Json> &bindings) {
	auto const doc = bindings->document();
	auto const contexts = doc.at("contexts");
	if (!contexts.is_array()) {
		LOG_ERROR(logger_) << "Missing contexts in bindings config";
		return false;
	}
	// for each context
	auto const items = contexts.as_array();
	auto it = items.begin();
	for (; it != items.end(); ++it) {
		if (!it->is_object()) {
			LOG_ERROR(logger_) << "Unrecognized context config";
			return false;
		}
		auto const context = it->as_object();
		auto const name = boost::json::value_to<std::string>(context.at("context"));
		auto binding = boost::make_shared<BindingContext>(name);

		auto const bindingsJson = context.at("bindings");
		if (!bindingsJson.is_object()) {
			LOG_ERROR(logger_) << "Missing bindings config";
			return false;
		}
		// for each binding
		auto const bindingsObj = bindingsJson.as_object();
		auto bindingsIter = bindingsObj.begin();
		for (; bindingsIter != bindingsObj.end(); ++bindingsIter) {
			std::string input = bindingsIter->key();
			// we're expecting string values only
			auto const actionValue = bindingsIter->value();
			if (!actionValue.is_string()) {
				LOG_ERROR(logger_) << "Unrecognized binding value type";
				return false;
			}
			binding->setBinding(input, actionValue.as_string());
		}
		LOG_INFO(logger_) << bindingsObj.size() << " bindings loaded";

		contexts_[name] = binding;
	}
	LOG_INFO(logger_) << contexts_.size() << " contexts loaded";

	return true;
}
