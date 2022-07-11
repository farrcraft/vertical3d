/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "StyleProperty.h"

#include <glm/glm.hpp>

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
			glm::vec3 color_;
	};

};
