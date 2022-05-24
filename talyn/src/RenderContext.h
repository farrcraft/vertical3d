/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

#include "FrameBuffer.h"

namespace Talyn
{
	/**
	 * What all does a rc encapsulate? What does the RISpec say about rendering with multiple renderers?
	 * Only one RC can be active at a time.
	 * The rc holds the current graphics state.
	 * How do you share common state between multiple rc's?
	 **/
	class RenderContext
	{
		public:
			RenderContext();

			/**
			 * Set the size of the framebuffer
			 * @param width the width of the image
			 * @param height the height of the image
			 */
			void format(unsigned int width, unsigned int height);

			void render();

			boost::shared_ptr<FrameBuffer> framebuffer() const;

		private:
			boost::shared_ptr<FrameBuffer> framebuffer_;
	};


}; // end namespace Talyn
