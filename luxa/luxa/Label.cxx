/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "Label.h"
#include "ComponentRenderer.h"

#include <vertical3d/gl/GLFontRenderer.h>

using namespace Luxa;

Label::Label(ComponentManager *cm) : Component(cm)
{
}

Label::~Label()
{
}

void Label::text(const std::string & txt)
{
	text_ = txt;
}

std::string Label::text(void) const
{
	return text_;
}

void Label::draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const
{
	glm::vec2 pos = position();

	boost::shared_ptr<v3D::Font2D> font = renderer->getDefaultFont("label", theme);
	if (!font)
	{
		return;
	}
	v3D::GLFontRenderer fr(*font);
	fr.print(text_, pos[0], pos[1]);
}

