/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Container.h"

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

	class Engine {
	public:
	private:
		std::vector<boost::shared_ptr<Container>> containers_;
	};

};  // namespace v3d::ui
