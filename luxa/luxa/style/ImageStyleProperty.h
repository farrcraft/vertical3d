/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_IMAGESTYLEPROPERTY
#define INCLUDED_LUXA_IMAGESTYLEPROPERTY

#include "StyleProperty.h"

#include <vertical3d/gl/GLTexture.h>

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
			boost::shared_ptr<v3D::GLTexture> texture(void) const;
			/**
			 * Get the name of the image source
			 * @return the image source name
			 */
			std::string source() const;
			/**
			 * Set the texture object associated with the image property
			 * @param tex the texture
			 */
			void texture(boost::shared_ptr<v3D::GLTexture> tex);

		private:
			std::string source_;
			boost::shared_ptr<v3D::GLTexture> texture_;
	};

}; // end namespace Luxa


#endif // INCLUDED_LUXA_IMAGESTYLEPROPERTY
