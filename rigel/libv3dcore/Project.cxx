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
#include "Project.h"

using namespace v3D;

Project::Project() : _selection(0), _activeTool(0)
{
	ScenePtr scene(new Scene());
	_scenes.push_back(scene);
	scene->active(true);
	_activeScene = scene;
}

Project::~Project()
{
}

Project & Project::instance(void)
{
	static Project instance;
	return instance;
}

ScenePtr Project::activeScene(void) const
{
	return _activeScene;
}

void Project::activeScene(ScenePtr scene)
{
	if (_activeScene)
		_activeScene->active(false);
	scene->active(true);
	_activeScene = scene;
}

void Project::addScene(ScenePtr scene)
{
	_scenes.push_back(scene);
}

void Project::clear(void)
{
	_name = "";
	std::vector<ScenePtr>::iterator it = _scenes.begin();
	for (; it != _scenes.end(); it++)
	{
		delete (*it);
	}
	_scenes.clear();
	_activeScene = 0;
}

void Project::name(const std::string & name)
{
	_name = name;
}

std::string Project::name(void) const
{
	return _name;
}

std::vector<ScenePtr> & Project::scenes(void)
{
	return _scenes;
}

Tool * Project::activeTool(void) const
{
	return _activeTool;
}

void Project::activateTool(Tool * tool)
{
	/*
	if (_activeTool && (_activeTool != tool))
		delete _activeTool;
	*/
	_activeTool = tool;
}

void Project::deactivateTool(void)
{
//	delete _activeTool;
	_activeTool = 0;
}
