/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include "../font/TextureTextBuffer.h"
#include "VertexBuffer.h"
#include "Program.h"
#include "GLTexture.h"

namespace v3D
{

	class TextureAtlas;

	/**
	 * A OpenGL Font renderer for texture fonts.
	 */
	class TextureFontRenderer
	{
		public:
			TextureFontRenderer();
			TextureFontRenderer(boost::shared_ptr<TextureTextBuffer> buffer, boost::shared_ptr<Program> program, boost::shared_ptr<TextureAtlas> atlas);
			virtual ~TextureFontRenderer();

			/**
			 * Access the underlying text buffer
			 */
			boost::shared_ptr<TextureTextBuffer> buffer();

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
			boost::shared_ptr<TextureTextBuffer> buffer_;
			boost::shared_ptr<TextureAtlas> atlas_;
			boost::shared_ptr<Program> program_;
			VertexBuffer vertexBuffer_;
			GLTexture texture_;
	};

}; // end namespace v3D

