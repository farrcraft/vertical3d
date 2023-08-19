/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mapper.h"

namespace v3d::event {

Mapper::Mapper(const std::string& name) : name_(name) {
}

std::string_view Mapper::name() const {
	return name_;
}

void Mapper::map(const Event& source, const Event& destination) {
	mappings_[source] = destination;
}

boost::optional<Event> Mapper::destination(const Event& source) {
	auto iter = mappings_.find(source);
	if (iter != mappings_.end()) {
		return iter->second;
	}
	return boost::none;
}

};  // namespace v3d::event
