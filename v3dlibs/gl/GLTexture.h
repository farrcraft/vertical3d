/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../image/Texture.h"

namespace v3d::gl {

	/**
	 * A OpenGL Texture object.
	 */
	class GLTexture : public v3d::image::Texture
	{
		public:
			GLTexture();
			GLTexture(const v3d::image::Texture &t);
			GLTexture(boost::shared_ptr<v3d::image::Image> image);
			virtual ~GLTexture();

			/**
			 * Make the texture active.
			 * @return true if successful otherwise false
			 */
			bool bind();
			/**
			 * Set whether the texture wraps (repeats)
			 * @param repeat whether the texture should wrap
			 */
			void wrap(bool repeat);
			/**
			 * Create a texture from the source image
			 * @param image the texture source image
			 * @return true if the texture was sucessfully created
			 */
			bool create(boost::shared_ptr<v3d::image::Image> image);
	};

};
