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
#include "TransformCommandSet.h"

#include <libv3dcommand/CommandDirectory.h>

#include "../Window.h"

using namespace v3D;

TransformCommandSet::TransformCommandSet()
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "transform::select");
	dir.connect(this, "transform::translate");
	dir.connect(this, "transform::rotate");
	dir.connect(this, "transform::scale");

	dir.connect(this, "select::mask::face");
	dir.connect(this, "select::mask::edge");
	dir.connect(this, "select::mask::vertex");
	dir.connect(this, "select::mask::curve");
	dir.connect(this, "select::mask::mesh");
	dir.connect(this, "select::mask::object");
	dir.connect(this, "select::mask::light");
	dir.connect(this, "select::mask::camera");
	dir.connect(this, "select::mask::handle");
}

TransformCommandSet::~TransformCommandSet()
{
}

bool TransformCommandSet::exec(const std::string & cmd)
{
	if (cmd == "transform::select")
	{
		Window::instance()->activeToolMode(Window::SELECT_TOOL_MODE);
	}
	else if (cmd == "transform::translate")
	{
		Window::instance()->activeToolMode(Window::TRANSLATE_TOOL_MODE);
	}
	else if (cmd == "transform::rotate")
	{
		Window::instance()->activeToolMode(Window::ROTATE_TOOL_MODE);
	}
	else if (cmd == "transform::scale")
	{
		Window::instance()->activeToolMode(Window::SCALE_TOOL_MODE);
	}
	else if (cmd == "select::mask::face")
	{
		Window::instance()->selectMasks(Window::SELECT_MASK_FACES);
	}
	else if (cmd == "select::mask::edge")
	{
		Window::instance()->selectMasks(Window::SELECT_MASK_EDGES);
	}
	else if (cmd == "select::mask::vertex")
	{
		Window::instance()->selectMasks(Window::SELECT_MASK_POINTS);
	}
	else if (cmd == "select::mask::curve")
	{
	}
	else if (cmd == "select::mask::mesh")
	{
	}
	else if (cmd == "select::mask::object")
	{
	}
	else if (cmd == "select::mask::light")
	{
	}
	else if (cmd == "select::mask::camera")
	{
	}
	else if (cmd == "select::mask::handle")
	{
	}
	else
	{
		return false;
	}
	return true;
}
