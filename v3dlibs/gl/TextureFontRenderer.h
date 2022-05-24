/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../font/TextureTextBuffer.h"
#include "../image/TextureAtlas.h"

#include "VertexBuffer.h"
#include "Program.h"
#include "GLTexture.h"

namespace v3d::gl {
	/**
	 * A OpenGL Font renderer for texture fonts.
	 */
	class TextureFontRenderer
	{
		public:
			TextureFontRenderer();
			TextureFontRenderer(boost::shared_ptr<v3d::font::TextureTextBuffer> buffer, boost::shared_ptr<Program> program, boost::shared_ptr<v3d::image::TextureAtlas> atlas);
			virtual ~TextureFontRenderer() = default;

			/**
			 * Access the underlying text buffer
			 */
			boost::shared_ptr<v3d::font::TextureTextBuffer> buffer();

			/**
			 * Upload font data to the GPU
			 */
			void upload();

			/**
			 * Render uploaded font data
			 */
			void render();

			/**
			 * Update the size of the rendering area.
			 * The underlying shader normal matrices will updated.
			 */
			void resize(float width, float height);

		private:
			boost::shared_ptr<v3d::font::TextureTextBuffer> buffer_;
			boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
			boost::shared_ptr<Program> program_;
			VertexBuffer vertexBuffer_;
			GLTexture texture_;
	};

}; // end namespace v3D

