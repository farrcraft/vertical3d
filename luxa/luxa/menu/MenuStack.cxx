/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "MenuStack.h"
#include "../ComponentRenderer.h"
#include "../style/Theme.h"
#include "../style/Style.h"
#include "../style/FontStyleProperty.h"

#include <vertical3d/gl/GLFontRenderer.h>

using namespace Luxa;

MenuStack::MenuStack(ComponentManager * cm) : Menu(cm)
{
}

MenuStack::~MenuStack()
{
}

void MenuStack::draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const
{
	renderer->clear();
	renderer->color(glm::vec3(0.75f, 0.75f, 0.75f));
	// locally formatted label string
	std::string label;
	boost::shared_ptr<Menu> lvl = level();
	boost::shared_ptr<MenuItem> active_item = lvl->active();
	size_t active_count = 0;

	boost::shared_ptr<v3D::Font2D> font = renderer->getDefaultFont("menu", theme);
	if (!font)
		return;

	// get total menu height
	size_t menu_height = font->height() * level()->size();
	size_t menu_count = level()->size() - 1;

	float x = (renderer->width() / 3.0f);
	float y = 0.0f;

	v3D::GLFontRenderer fr(*font);

	// iterate over the active menu level
	for (int i = menu_count; i >= 0; i--)
	{
		y = (((renderer->height() - menu_height) / 2.0f) + (i * 50.0f));
		label = (*lvl)[i]->label();

		// mark active menu item
		if (active_item && active_item->id() == (*lvl)[i]->id())
		{
			active_count = i;
		}
		fr.print(label, x, y);
	}

	// draw active item marker
	if (active_item)
	{
		label = " > ";
		x -= font->width(label);
		y = (((renderer->height() - menu_height) / 2.0f) + (active_count * 50.0f));
		fr.print(label, x, y);
	}
}
