/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Menu.h"

namespace Luxa
{

	/**
	 * A MenuStack implements a container of vertically stacked menu items.
	 * This type of menu would commonly be used for in-game menu systems.
	 **/
	class MenuStack : public Menu
	{
		public:
			MenuStack(ComponentManager * cm);
			virtual ~MenuStack();

			/**
			 * Draw the menu stack.
			 * @param renderer the renderer to use for drawing
			 * @param theme the active theme
			 */
			void draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const;

	};

}; // end namespace Luxa
