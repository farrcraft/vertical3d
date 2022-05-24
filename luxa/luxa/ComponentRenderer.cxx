/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ComponentRenderer.h"
#include "style/FontStyleProperty.h"
#include "style/Style.h"

#include <gl/glew.h>

#include <boost/make_shared.hpp>

using namespace Luxa;

ComponentRenderer::ComponentRenderer(boost::shared_ptr<v3d::font::FontCache> fc) : fonts_(fc)
{
}

ComponentRenderer::ComponentRenderer() : width_(-1), height_(-1) {
	fonts_ = boost::make_shared<v3d::font::FontCache>();

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

ComponentRenderer::~ComponentRenderer()
{
}

void ComponentRenderer::resize(int width, int height)
{
	GLfloat ratio;

	if (height == 0)
		height = 1;

	width_ = width;
	height_ = height;

	ratio = (GLfloat)width / (GLfloat)height;

	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    glOrtho(0, width, 0, height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void ComponentRenderer::overlay(const Overlay & o)
{
	overlay_ = o;
}

void ComponentRenderer::prepare()
{
	if (overlay_.mode() == Overlay::MODE_COLOR)
	{
		v3d::type::Color3 overlay_color = overlay_.color();
		glClearColor(overlay_color[0], overlay_color[1], overlay_color[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glPushAttrib(GL_FOG_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);

//	glFrontFace(GL_CCW);
//	glCullFace(GL_BACK);
//	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// setup matrices
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	// map top left to (0,0)
	glOrtho(0, (float)width_, (float)height_, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
}

void ComponentRenderer::post()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void ComponentRenderer::draw()
{
}

/*
the alternative to directly rendering the quad here would be to store it for batch rendering later.


class TexturedQuad
{
	public:
		boost::shared_ptr<Hookah::Texture> _texture;
		v3D::Vector2 _position;
		v3D::Vector2 _size;
};
*/

bool ComponentRenderer::drawTexturedQuad(boost::shared_ptr<v3d::gl::GLTexture> texture, float position_x, float position_y, float width, float height)
{
	// get texturing state
	GLboolean texture_enabled;
	texture_enabled = glIsEnabled(GL_TEXTURE_2D);
	// enable states if they weren't already
	if (texture_enabled == GL_FALSE)
		glEnable(GL_TEXTURE_2D);

	if (!texture)
	{
		return false;
	}
	texture->bind();

	glPushMatrix();
	glTranslatef(position_x, position_y, 0.0f);

	glScalef(width, height, 1.0f);

	// draw quad
	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f); 
	glVertex3f(0.0f,  1.0f,  0.0f);

	glTexCoord2f(1.0f, 0.0f); 
	glVertex3f(1.0f,  1.0f,  0.0f);

	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.0f, 0.0f,  0.0f);

	glTexCoord2f(0.0f, 1.0f); 
	glVertex3f(0.0f, 0.0f,  0.0f);

	glEnd();

	glPopMatrix();

	// reset texture state to initial setting
	if (texture_enabled == GL_FALSE)
		glDisable(GL_TEXTURE_2D);

	return true;
}

bool ComponentRenderer::drawTexture(boost::shared_ptr<v3d::gl::GLTexture> texture, glm::vec2 position)
{
	// get texturing state
	GLboolean texture_enabled;
	texture_enabled = glIsEnabled(GL_TEXTURE_2D);
	// enable states if they weren't already
	if (texture_enabled == GL_FALSE)
	{
		glEnable(GL_TEXTURE_2D);
	}

	if (!texture)
	{
		return false;
	}
	texture->bind();

	glPushMatrix();
	glTranslatef(position[0], position[1], 0.0f);

	float x = static_cast<float>(texture->width());
	float y = static_cast<float>(texture->height());
	// draw quad
	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 1.0f); 
	glVertex3f(0.0f,  y,  0.0f);

	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(x,  y,  0.0f);

	glTexCoord2f(1.0f, 0.0f); 
	glVertex3f(x, 0.0f,  0.0f);

	glTexCoord2f(0.0f, 0.0f); 
	glVertex3f(0.0f, 0.0f,  0.0f);

	glEnd();

	glPopMatrix();

	// reset texture state to initial setting
	if (texture_enabled == GL_FALSE)
	{
		glDisable(GL_TEXTURE_2D);
	}

	return true;
}

void ComponentRenderer::position(const glm::vec2 & pos)
{
	glTranslatef(pos[0], pos[1], 0.0f);
}

void ComponentRenderer::push()
{
	glPushMatrix();
}

void ComponentRenderer::pop()
{
	glPopMatrix();
}

void ComponentRenderer::clear()
{
	glLoadIdentity();
}

boost::shared_ptr<v3d::font::FontCache> ComponentRenderer::fonts() const
{
	return fonts_;
}

boost::shared_ptr<v3d::font::Font2D> ComponentRenderer::getDefaultFont(const std::string & style_class, boost::shared_ptr<Theme> theme)
{
	boost::shared_ptr<v3d::font::Font2D> font;

	// get the default class style
	std::vector< boost::shared_ptr<Style> > style_set = theme->getStyleSet("default", style_class);
	if (style_set.empty())
	{
		return font;
	}

	// get the font name from the label style property
	boost::shared_ptr<FontStyleProperty> font_prop = 
		boost::dynamic_pointer_cast<FontStyleProperty, StyleProperty>(style_set[0]->property("label", "font"));
	font = fonts_->get(font_prop->source());

	return font;
}

unsigned int ComponentRenderer::width() const
{
	return width_;
}

unsigned int ComponentRenderer::height() const
{
	return height_;
}

void ComponentRenderer::color(const glm::vec3 & c)
{
	glColor3f(c[0], c[1], c[2]);
}
