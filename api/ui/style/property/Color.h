/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Property.h"

#include <glm/glm.hpp>

namespace v3d::ui::style::prop
{

	/**
	 * A vGUI style property that defines a single color.
	 */
	class Color : public Property
	{
	public:
		Color();
		~Color();

	private:
		glm::vec3 color_;
	};

};
