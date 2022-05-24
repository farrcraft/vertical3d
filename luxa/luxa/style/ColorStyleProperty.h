/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "StyleProperty.h"

#include "../../../v3dlibs/type/Color3.h"

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
			v3d::type::Color3 color_;
	};

};
