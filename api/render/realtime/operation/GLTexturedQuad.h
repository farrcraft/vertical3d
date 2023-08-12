/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../../../gl/GLTexture.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::operation {
	/**
	 * Draw a GL textured quad at position of a given dimensions (width, height)
	 **/
	class GLTexturedQuad : public Operation {
	public:
		/**
		 *
		 **/
		GLTexturedQuad(boost::shared_ptr<v3d::gl::GLTexture> texture, const glm::vec2& position, const glm::vec2& dimensions);

		/**
		 *
		 **/
		bool run(boost::shared_ptr<Context> context);

	private:
		glm::vec2 position_;
		glm::vec2 dimensions_;
		boost::shared_ptr<v3d::gl::GLTexture> texture_;
	};
};
