/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */
#pragma once

#include "../font/BitmapTextBuffer.h"
#include "VertexBuffer.h"
#include "Program.h"
#include "GLTexture.h"

namespace v3D
{

	/**
	 * A OpenGL Font renderer.
	 */
	class BitmapFontRenderer
	{
		public:
			BitmapFontRenderer();
			BitmapFontRenderer(boost::shared_ptr<BitmapTextBuffer> buffer, boost::shared_ptr<Program> program);
			virtual ~BitmapFontRenderer();

			boost::shared_ptr<BitmapTextBuffer> buffer();

			void upload();
			void render();
			void clear();

		private:
			boost::shared_ptr<BitmapTextBuffer> buffer_;
			boost::shared_ptr<Program> program_;
			VertexBuffer vertexBuffer_;
			GLTexture 	texture_;
	};

}; // end namespace v3D
