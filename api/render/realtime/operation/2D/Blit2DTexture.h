/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../../Operation.h"
#include "../../2D/Texture2D.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::operation {
	/**
	 * Copy a source texture to a destination texture
	 **/
	class Blit2DTexture : public Operation {
	public:
		/**
		 **/
		Blit2DTexture(boost::shared_ptr<Texture2D> source, boost::shared_ptr<Texture2D> destination);
		
		/**
		 **/
		bool run(boost::shared_ptr<Context2D> context);

	private:
		boost::shared_ptr<Texture2D> source_;
		boost::shared_ptr<Texture2D> destination_;
	};
};
