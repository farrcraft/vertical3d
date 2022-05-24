/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../font/BitmapTextBuffer.h"
#include "VertexBuffer.h"
#include "Program.h"
#include "GLTexture.h"

namespace v3d::gl {

	/**
	 * A OpenGL Font renderer.
	 */
	class BitmapFontRenderer final
	{
		public:
			BitmapFontRenderer();
			BitmapFontRenderer(boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer, boost::shared_ptr<Program> program);
			virtual ~BitmapFontRenderer() = default;

			boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer();

			void upload();
			void render();
			void clear();

		private:
			boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer_;
			boost::shared_ptr<Program> program_;
			VertexBuffer vertexBuffer_;
			GLTexture 	texture_;
	};

};