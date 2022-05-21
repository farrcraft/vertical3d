/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "Button.h"
#include "ComponentManager.h"
#include "style/ButtonStyle.h"
#include "style/ImageStyleProperty.h"
#include "style/FontStyleProperty.h"

#include <vertical3d/gl/GLFontRenderer.h>

using namespace Luxa;

Button::Button(ComponentManager * cm) : Component(cm), state_(STATE_NORMAL)
{
}

void Button::label(const std::string & str)
{
	label_ = str;
}

std::string Button::label() const
{
	return label_;
}

Button::ButtonState Button::state() const
{
	return state_;
}

void Button::state(ButtonState s)
{
	state_ = s;
}

void Button::notify(const v3D::EventInfo & e)
{
	if (e.name() == "enter")
	{
		state_ = STATE_HOVER;
	}
	else if (e.name() == "leave")
	{
		state_ = STATE_NORMAL;
	}
	else if (e.name() == "button_pressed")
	{
		state_ = STATE_PRESS;
	}
	else if (e.name() == "button_released")
	{
		state_ = STATE_HOVER;
	}
}


void Button::draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const
{
	renderer->push();
	renderer->position(position());

	// get the style for the button in its current state
	// first get all of the default button styles
	// where should "default" come from? it probably shouldn't be hardcoded here.
	std::vector< boost::shared_ptr<Style> > style_set = theme->getStyleSet("default", "button");
	// get the style matching this state
	boost::shared_ptr<ButtonStyle> style;
	std::vector< boost::shared_ptr<Style> >::iterator iter = style_set.begin();
	for (; iter != style_set.end(); iter++)
	{
		style = boost::dynamic_pointer_cast<ButtonStyle, Style>(*iter);
		if (style->state() == state_)
		{
			break;
		}
		style.reset();
	}
	// no style for default button with this state
	if (!style)
		return;

	boost::shared_ptr<ImageStyleProperty> prop;
	boost::shared_ptr<v3D::GLTexture> texture, left_texture, right_texture, top_texture, bottom_texture;

	// preload the 4 side textures first so we can calculate the proper button dims later
	// left
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("left", "image"));
	left_texture = prop->texture();
	float left_width = static_cast<float>(left_texture->width());
	// right
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("right", "image"));
	right_texture = prop->texture();
	float right_width = static_cast<float>(right_texture->width());
	// top
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("top", "image"));
	top_texture = prop->texture();
	top_texture->wrap(true);
	float top_height = static_cast<float>(top_texture->height());
	// bottom
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("bottom", "image"));
	bottom_texture = prop->texture();
	bottom_texture->wrap(true);
	float bottom_height = static_cast<float>(bottom_texture->height());

	// get default button dims
	glm::vec2 s = size();
	float button_width = s[0];
	float button_height = s[1];

	boost::shared_ptr<v3D::Font2D> font = renderer->getDefaultFont("button", theme);
	// make sure label text will fit within the button and expand the button size to make it fit if necessary
	unsigned int font_width = font->width(label_);
	unsigned int font_height = font->height();
	if (button_width <= font_width)
		button_width = font_width + left_width + right_width;
	if (button_height <= font_height)
		button_height = font_height + top_height + bottom_height;

	// draw 8 button sides + interior

	// interior
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("background", "image"));
	texture = prop->texture();
	texture->wrap(true);
	renderer->drawTexturedQuad(texture, 0., 0., button_width, button_height);

	// left
	renderer->drawTexturedQuad(left_texture, 0., 0., left_width, button_height);
	// right
	renderer->drawTexturedQuad(right_texture, button_width - right_width, 0., right_width, button_height);
	// top
	renderer->drawTexturedQuad(top_texture, 0., 0., button_width, top_height);
	// bottom
	renderer->drawTexturedQuad(bottom_texture, 0., button_height - bottom_height, button_width, bottom_height);

	// top-right
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("top-right", "image"));
	texture = prop->texture();
	float top_right_width = static_cast<float>(texture->width());
	float top_right_height = static_cast<float>(texture->height());
	renderer->drawTexturedQuad(texture, button_width - top_right_width, 0., top_right_width, top_right_height);

	// top-left
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("top-left", "image"));
	texture = prop->texture();
	float top_left_width = static_cast<float>(texture->width());
	float top_left_height = static_cast<float>(texture->height());
	renderer->drawTexturedQuad(texture, 0., 0., top_left_width, top_left_height);

	// bottom-left
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("bottom-left", "image"));
	texture = prop->texture();
	float bottom_left_width = static_cast<float>(texture->width());
	float bottom_left_height = static_cast<float>(texture->height());
	renderer->drawTexturedQuad(texture, 0., button_height - bottom_left_height, bottom_left_width, bottom_left_height);

	// bottom-right
	prop = boost::dynamic_pointer_cast<ImageStyleProperty, StyleProperty>(style->property("bottom-right", "image"));
	texture = prop->texture();
	float bottom_right_width = static_cast<float>(texture->width());
	float bottom_right_height = static_cast<float>(texture->height());
	renderer->drawTexturedQuad(texture, button_width - bottom_right_width, button_height - bottom_right_height, bottom_right_width, bottom_right_height);

	// draw button label
	// we need to figure out where to place the font cursor based on the specific font metrics and the size of the
	// text that will be drawn
	float button_mid_x = button_width / 2.0f;
	float button_mid_y = button_height / 2.0f;

	button_mid_x -= font_width / 2.0f;
	button_mid_y -= font_height / 2.0f;
	// cm->renderer()->print(font, button_mid_x, button_mid_y, _label)
	v3D::GLFontRenderer fr(*font);
	fr.print(label_, button_mid_x, button_mid_y);


	// return to parent coordinate system
	renderer->pop();
}

