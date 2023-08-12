/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../../../font/TextureTextBuffer.h"
#include "../../../image/TextureAtlas.h"
#include "../../../log/Logger.h"

#include "../../../gl/VertexBuffer.h"
#include "../../../gl/Program.h"
#include "../../../gl/GLTexture.h"

namespace v3d::render::realtime::operation {
	/**
	 * Draw a GL font at position
	 **/
	class TextureFont : public Operation {
	public:
		/**
		 *
		 **/
		TextureFont(boost::shared_ptr<v3d::font::TextureTextBuffer> buffer, boost::shared_ptr<v3d::gl::Program> program,
			boost::shared_ptr<v3d::image::TextureAtlas> atlas, const boost::shared_ptr<v3d::log::Logger>& logger);

		/**
		 * Access the underlying text buffer
		 */
		boost::shared_ptr<v3d::font::TextureTextBuffer> buffer();

		/**
		 * Update the size of the rendering area.
		 * The underlying shader normal matrices will updated.
		 */
		void resize(float width, float height);

		/**
		 *
		 **/
		bool run(boost::shared_ptr<Context> context);

	protected:
		/**
		 * Upload font data to the GPU
		 */
		void upload();

	private:
		boost::shared_ptr<v3d::font::TextureTextBuffer> buffer_;
		boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
		boost::shared_ptr<v3d::gl::Program> program_;
		v3d::gl::VertexBuffer vertexBuffer_;
		v3d::gl::GLTexture texture_;
		boost::shared_ptr<v3d::log::Logger> logger_;
	};
};
