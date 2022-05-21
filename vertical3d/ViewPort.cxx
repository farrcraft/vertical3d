#include "ViewPort.h"
#include <vertical3d/core/Scene.h>

using namespace v3D;

ViewPort::ViewPort(const boost::shared_ptr<Hookah::Window> & window, const boost::shared_ptr<Scene> & scene) : 
	window_(window),
	scene_(scene), 
	camera_(new Camera)
{
}

ViewPort::~ViewPort()
{
}

boost::shared_ptr<Camera> ViewPort::camera()
{
	return camera_;
}

void ViewPort::resize(int width, int height)
{
	rc_.resize(width, height);
}

void ViewPort::invalidate()
{
	window_->invalidate();
}

void ViewPort::draw(Hookah::Window * window)
{

	rc_.prepare(camera_);
/*
	if (showFlags_ & SHOW_GRID)
	{
		glDisable(GL_LIGHTING);
		glPushMatrix();
		_grid.draw(_camera);
		glPopMatrix();
		glEnable(GL_LIGHTING);
	}
*/
	rc_.render(scene_);
}

