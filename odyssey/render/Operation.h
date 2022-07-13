/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
	/**
	 **/
	class Operation {
	public:
		/**
		 **/
		virtual bool run(boost::shared_ptr<Context> context) = 0;
	};
};
