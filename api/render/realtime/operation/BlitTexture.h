/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../Texture.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::operation {
	/**
	 * Copy a source texture to a destination texture
	 **/
	class BlitTexture : public Operation {
	public:
		/**
		 **/
		BlitTexture(boost::shared_ptr<Texture> source, boost::shared_ptr<Texture> destination);
		
		/**
		 **/
		bool run(boost::shared_ptr<Context> context);

	private:
		boost::shared_ptr<Texture> source_;
		boost::shared_ptr<Texture> destination_;
	};
};
