/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../font/Font2D.h"
#include "GLTexture.h"

namespace v3d::gl
{

	/**
	 * A OpenGL Font renderer.
	 */
	class GLFontRenderer
	{
		public:
			GLFontRenderer();
			GLFontRenderer(const v3d::font::Font2D &f);
			virtual ~GLFontRenderer();

			/**
			 * 
			 * Render the the text to the current drawing surface
			 * @param text the text to be printed
			 * @param x the left position to print
			 * @param y the top of the position to begin printing
			 */
			void print(const std::string & text, float x, float y);


		private:
			v3d::font::Font2D font_;
			GLTexture texture_;
	};

};