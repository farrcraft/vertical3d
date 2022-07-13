/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Frame.h"
#include "renderable/Player.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
	/**
	 **/
	class Scene {
	public:
		/**
		 **/
		Scene(boost::shared_ptr<Context> context);

		/**
		 **/
		boost::shared_ptr<Frame> collect();

		/**
		 **/
		void setPlayer(boost::shared_ptr<renderable::Player> player);

	private:
		boost::shared_ptr<renderable::Player> player_;
		boost::shared_ptr<Context> context_;
	};
};
