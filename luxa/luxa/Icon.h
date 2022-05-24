/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Component.h"

#include "../../v3dlibs/gl/GLTexture.h"

#include <boost/shared_ptr.hpp>

namespace Luxa {

	class ComponentManager;

	/**
	 * An Icon Component.
	 * Icons consist of a single texture image. Their size comes from the texture's
	 * dimensions and not the size set in the component.
	 */
	class Icon : public Component
	{
		public:
			Icon(boost::shared_ptr<v3d::gl::GLTexture> texture, ComponentManager * cm);
			~Icon();

			/**
			 * Draw the icon
			 * @param renderer the renderer to use for drawing
			 * @param theme the active theme
			 */
			void draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const;

		private:
			boost::shared_ptr<v3d::gl::GLTexture> texture_;
	};


}; // end namespace Luxa
