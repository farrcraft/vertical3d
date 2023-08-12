/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Property.h"

#include "../../../api/gl/GLTexture.h"

#include <boost/shared_ptr.hpp>

#include <string>

namespace v3d::ui::style::prop {

	/**
	 * A style property that defines an image.
	 */
	class Image : public Property
	{
	public:
		Image(const std::string& name, const std::string& src);
		~Image();

		/**
		 * Get the texture associated with the image property
		 * @return a pointer to the texture
		 */
		boost::shared_ptr<v3d::gl::GLTexture> texture(void) const;
		/**
		 * Get the name of the image source
		 * @return the image source name
		 */
		std::string_view source() const;
		/**
		 * Set the texture object associated with the image property
		 * @param tex the texture
		 */
		void texture(boost::shared_ptr<v3d::gl::GLTexture> tex);

	private:
		std::string source_;
		boost::shared_ptr<v3d::gl::GLTexture> texture_;
	};

}; // end namespace v3d::ui::style::prop
