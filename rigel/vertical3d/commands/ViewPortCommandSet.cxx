/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "ViewPortCommandSet.h"

#include <libv3dcommand/CommandDirectory.h>

#include "../Window.h"

using namespace v3D;

ViewPortCommandSet::ViewPortCommandSet()
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "view::rendermode::front::points");
	dir.connect(this, "view::rendermode::front::lines");
	dir.connect(this, "view::rendermode::front::fill");
	dir.connect(this, "view::rendermode::back::points");
	dir.connect(this, "view::rendermode::back::lines");
	dir.connect(this, "view::rendermode::back::fill");
	dir.connect(this, "view::cull::none");
	dir.connect(this, "view::cull::front");
	dir.connect(this, "view::cull::back");
	dir.connect(this, "view::cull::both");
	dir.connect(this, "view::shade::flat");
	dir.connect(this, "view::shade::smooth");
	dir.connect(this, "view::show::handle");
	dir.connect(this, "view::show::camera");
	dir.connect(this, "view::show::light");
	dir.connect(this, "view::show::grid");
	dir.connect(this, "view::show::mesh");
	dir.connect(this, "view::camera::front");
	dir.connect(this, "view::camera::back");
	dir.connect(this, "view::camera::left");
	dir.connect(this, "view::camera::right");
	dir.connect(this, "view::camera::top");
	dir.connect(this, "view::camera::bottom");
	dir.connect(this, "view::camera::perspective");
	dir.connect(this, "view::camera::zoom");
	dir.connect(this, "view::camera::truck");
	dir.connect(this, "view::camera::pan");
	dir.connect(this, "view::pop");
	dir.connect(this, "view::display::wireframe");
	dir.connect(this, "view::display::shaded");
}

ViewPortCommandSet::~ViewPortCommandSet()
{
}

bool ViewPortCommandSet::exec(const std::string & cmd)
{
	if (cmd == "view::rendermode::front::points")
	{
		Window::instance()->activeView()->_frontRenderMode = ViewPort::POLY_RENDER_POINTS;
	}
	else if (cmd == "view::rendermode::front::lines")
	{
		Window::instance()->activeView()->_frontRenderMode = ViewPort::POLY_RENDER_LINES;
	}
	else if (cmd == "view::rendermode::front::fill")
	{
		Window::instance()->activeView()->_frontRenderMode = ViewPort::POLY_RENDER_FILL;
	}
	else if (cmd == "view::rendermode::back::points")
	{
		Window::instance()->activeView()->_backRenderMode = ViewPort::POLY_RENDER_POINTS;
	}
	else if (cmd == "view::rendermode::back::lines")
	{
		Window::instance()->activeView()->_backRenderMode = ViewPort::POLY_RENDER_LINES;
	}
	else if (cmd == "view::rendermode::back::fill")
	{
		Window::instance()->activeView()->_backRenderMode = ViewPort::POLY_RENDER_FILL;
	}
	else if (cmd == "view::cull::none")
	{
		Window::instance()->activeView()->_cullMode = ViewPort::POLY_CULL_NONE;
	}
	else if (cmd == "view::cull::front")
	{
		Window::instance()->activeView()->_cullMode = ViewPort::POLY_CULL_FRONT;
	}
	else if (cmd == "view::cull::back")
	{
		Window::instance()->activeView()->_cullMode = ViewPort::POLY_CULL_BACK;
	}
	else if (cmd == "view::cull::both")
	{
		Window::instance()->activeView()->_cullMode = ViewPort::POLY_CULL_BOTH;
	}
	else if (cmd == "view::shade::flat")
	{
		Window::instance()->activeView()->_shadeMode = ViewPort::POLY_SHADE_FLAT;
	}
	else if (cmd == "view::shade::smooth")
	{
		Window::instance()->activeView()->_shadeMode = ViewPort::POLY_SHADE_SMOOTH;
	}
	else if (cmd == "view::show::handle")
	{
		Window::instance()->activeView()->_showFlags ^= ViewPort::SHOW_HANDLE;
	}
	else if (cmd == "view::show::camera")
	{
		Window::instance()->activeView()->_showFlags ^= ViewPort::SHOW_CAMERA;
	}
	else if (cmd == "view::show::light")
	{
		Window::instance()->activeView()->_showFlags ^= ViewPort::SHOW_LIGHT;
	}
	else if (cmd == "view::show::grid")
	{
		Window::instance()->activeView()->_showFlags ^= ViewPort::SHOW_GRID;
	}
	else if (cmd == "view::show::mesh")
	{
		Window::instance()->activeView()->_showFlags ^= ViewPort::SHOW_MESH;
	}
	else if (cmd == "view::camera::front")
	{
		Window::instance()->activeView()->activeCamera("Front");
	}
	else if (cmd == "view::camera::back")
	{
		Window::instance()->activeView()->activeCamera("Back");
	}
	else if (cmd == "view::camera::left")
	{
		Window::instance()->activeView()->activeCamera("Left");
	}
	else if (cmd == "view::camera::right")
	{
		Window::instance()->activeView()->activeCamera("Right");
	}
	else if (cmd == "view::camera::top")
	{
		Window::instance()->activeView()->activeCamera("Top");
	}
	else if (cmd == "view::camera::bottom")
	{
		Window::instance()->activeView()->activeCamera("Bottom");
	}
	else if (cmd == "view::camera::perspective")
	{
		Window::instance()->activeView()->activeCamera("Perspective");
	}
	else if (cmd == "view::camera::zoom")
	{
		Window::instance()->activeView()->cameraMode(ViewPort::ZOOM_CAMERA_MODE);
	}
	else if (cmd == "view::camera::truck")
	{
		Window::instance()->activeView()->cameraMode(ViewPort::TRUCK_CAMERA_MODE);
	}
	else if (cmd == "view::camera::pan")
	{
		Window::instance()->activeView()->cameraMode(ViewPort::PAN_CAMERA_MODE);
	}
	else if (cmd == "view::pop")
	{
		Window::instance()->layout()->pop(Window::instance()->activeView());
	}
	else if (cmd == "view::display::shaded")
	{
		Window::instance()->activeView()->_frontRenderMode = ViewPort::POLY_RENDER_FILL;
		Window::instance()->activeView()->_backRenderMode = ViewPort::POLY_RENDER_FILL;
	}
	else if (cmd == "view::display::wireframe")
	{
		Window::instance()->activeView()->_frontRenderMode = ViewPort::POLY_RENDER_LINES;
		Window::instance()->activeView()->_backRenderMode = ViewPort::POLY_RENDER_LINES;
	}
	else
	{
		return false;
	}
	return true;
}
