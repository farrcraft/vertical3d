/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Operation.h"

#include <vector>

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
	/**
	 **/
	class Frame {
	public:
		/**
		 **/
		Frame(boost::shared_ptr<Context> context);

		/**
		 **/
		void addOperation(boost::shared_ptr<Operation> operation);

		/**
		 **/
		void draw();

	private:
		boost::shared_ptr<Context> context_;
		std::vector<boost::shared_ptr<Operation>> operations_;
	};
};
