/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../api/ecs/System.h"

namespace odyssey::system {
	/**
	 **/
	class Movement final : public v3d::ecs::System {
	public:
		/**
		 * @return bool
		 **/
		bool tick();
	};
};
