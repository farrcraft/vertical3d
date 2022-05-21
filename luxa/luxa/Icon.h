/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_ICON
#define INCLUDED_LUXA_ICON

#include "Component.h"

#include <vertical3d/gl/GLTexture.h>

#include <boost/shared_ptr.hpp>

namespace Luxa
{

	class ComponentManager;

	/**
	 * An Icon Component.
	 * Icons consist of a single texture image. Their size comes from the texture's
	 * dimensions and not the size set in the component.
	 */
	class Icon : public Component
	{
		public:
			Icon(boost::shared_ptr<v3D::GLTexture> texture, ComponentManager * cm);
			~Icon();

			/**
			 * Draw the icon
			 * @param renderer the renderer to use for drawing
			 * @param theme the active theme
			 */
			void draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const;

		private:
			boost::shared_ptr<v3D::GLTexture> texture_;
	};


}; // end namespace Luxa


#endif // INCLUDED_LUXA_ICON
