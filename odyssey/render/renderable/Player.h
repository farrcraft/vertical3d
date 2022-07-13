/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Renderable.h"
#include "../../engine/Player.h"

namespace odyssey::render::renderable {
	/**
	 **/
	class Player : public Renderable {
	public:
		/**
		 **/
		Player(boost::shared_ptr<Engine> renderer, boost::shared_ptr<odyssey::engine::Player> player);

		/**
		 **/
		void draw(boost::shared_ptr<Frame> frame);

	private:
		boost::shared_ptr<odyssey::engine::Player> player_;
	};
};
