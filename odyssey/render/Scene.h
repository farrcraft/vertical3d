/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../api/render/realtime/Context.h"
#include "../../api/render/realtime/Frame.h"
#include "renderable/Player.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::render {
	/**
	 **/
	class Scene {
	public:
		/**
		 **/
		Scene(boost::shared_ptr<v3d::render::realtime::Context> context);

		/**
		 **/
		boost::shared_ptr<v3d::render::realtime::Frame> collect();

		/**
		 **/
		void setPlayer(boost::shared_ptr<renderable::Player> player);

	private:
		boost::shared_ptr<renderable::Player> player_;
		boost::shared_ptr<v3d::render::realtime::Context> context_;
	};
};
