/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Mapper.h"

#include <boost/shared_ptr.hpp>

namespace v3d::event {
	/**
	 **/
	class Engine {
	public:
		void addMapper(const boost::shared_ptr<Mapper>& mapper);

	private:
		std::map<std::string, boost::shared_ptr<Mapper>> mappers_;
	};

};  // namespace v3d::event
