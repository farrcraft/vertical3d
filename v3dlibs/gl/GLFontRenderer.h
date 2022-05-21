/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#ifndef INCLUDED_V3D_GL_FONT_RENDERER
#define INCLUDED_V3D_GL_FONT_RENDERER

#include "../font/Font2D.h"
#include "GLTexture.h"

namespace v3D
{

	/**
	 * A OpenGL Font renderer.
	 */
	class GLFontRenderer
	{
		public:
			GLFontRenderer();
			GLFontRenderer(const Font2D &f);
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
			Font2D 		font_;
			GLTexture 	texture_;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_GL_TEXTURE
