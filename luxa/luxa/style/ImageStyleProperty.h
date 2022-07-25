/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "StyleProperty.h"

#include "../../../api/gl/GLTexture.h"

#include <boost/shared_ptr.hpp>

#include <string>

namespace Luxa
{

	/**
	 * A style property that defines an image.
	 */
	class ImageStyleProperty : public StyleProperty
	{
		public:
			ImageStyleProperty(const std::string & name, const std::string & src);
			~ImageStyleProperty();

			/**
			 * Get the texture associated with the image property
			 * @return a pointer to the texture
			 */
			boost::shared_ptr<v3d::gl::GLTexture> texture(void) const;
			/**
			 * Get the name of the image source
			 * @return the image source name
			 */
			std::string source() const;
			/**
			 * Set the texture object associated with the image property
			 * @param tex the texture
			 */
			void texture(boost::shared_ptr<v3d::gl::GLTexture> tex);

		private:
			std::string source_;
			boost::shared_ptr<v3d::gl::GLTexture> texture_;
	};

}; // end namespace Luxa
