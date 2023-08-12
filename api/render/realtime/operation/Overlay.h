/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../../../ui/Overlay.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::operation {
	/**
	 * Draw a GL texture at position
	 **/
	class Overlay : public Operation {
	public:
		/**
		 *
		 **/
		Overlay(boost::shared_ptr<v3d::ui::Overlay> overlay);

		/**
		 *
		 **/
		bool run(boost::shared_ptr<Context> context);

	private:
		boost::shared_ptr<v3d::ui::Overlay> overlay_;
	};
};
