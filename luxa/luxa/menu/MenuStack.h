/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_MENUSTACK
#define INCLUDED_LUXA_MENUSTACK

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


#endif // INCLUDED_LUXA_MENUSTACK
