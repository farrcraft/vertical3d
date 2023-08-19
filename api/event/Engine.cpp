/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

namespace v3d::event {
	void Engine::addMapper(const boost::shared_ptr<Mapper>& mapper) {
		std::string mapperName(mapper->name());
		mappers_[mapperName] = mapper;
	}

};  // namespace v3d::event
