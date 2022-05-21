/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_COLORSTYLEPROPERTY
#define INCLUDED_LUXA_COLORSTYLEPROPERTY

#include "StyleProperty.h"

#include <vertical3d/3dtypes/Color3.h>

namespace Luxa
{

	/**
	 * A vGUI style property that defines a single color.
	 */
	class ColorStyleProperty : public StyleProperty
	{
		public:
			ColorStyleProperty();
			~ColorStyleProperty();

		private:
			v3D::Color3 color_;
	};

};

#endif // INCLUDED_LUXA_COLORSTYLEPROPERTY
