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

#include <gtkmm.h>
#include <gtkglmm.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <GL/gl.h>

#include <iostream>

// v3D includes
#include "Window.h"

#include "commands/CreatePolyCommandSet.h"
#include "commands/ViewPortCommandSet.h"
#include "commands/TransformCommandSet.h"
#include "commands/ProjectCommandSet.h"
#include "commands/PolyTools.h"

using namespace v3D;

// initialize commands
CreatePolyCommandSet	create_poly_commands;
ViewPortCommandSet 		viewport_commands;
TransformCommandSet		transform_commands;
ProjectCommandSet		project_commands;

// initialize tools
SplitEdgeTool			split_edge_tool;
SplitFaceTool			split_face_tool;
ExtrudeFaceTool			extrude_face_tool;

// initialize debug log
slug::logstream 		_log;

// main entry point
int main (int argc, char *argv[])
{
	_log.open("vertical3d.log");
	_log.filter(slug::log_base::ll_error | slug::log_base::ll_warning | slug::log_base::ll_info);
	_log.level(slug::log_base::ll_info) << "Vertical|3D Logging Enabled.\n";

	// initialize gtkmm
	_log.level(slug::log_base::ll_info) << "initializing gtkmm...\n";
	Gtk::Main kit(argc, argv);

	// initialize gtkglextmm
	_log.level(slug::log_base::ll_info) << "initializing gtkglextmm...\n";
	Gtk::GL::init(argc, argv);

	// create window
	_log.level(slug::log_base::ll_info) << "creating window...\n";
	_log.flush();
	Window * win = Window::instance();
	win->initialize();

	// start main event loop
	_log.level(slug::log_base::ll_info) << "entering main event loop...\n";
	kit.run(*win);

	// cleanup and exit
	_log.level(slug::log_base::ll_info) << "shutting down...\n";
	delete Window::instance();

	return 0;
}
