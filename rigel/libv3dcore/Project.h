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
#ifndef INCLUDED_V3D_PROJECT_H
#define INCLUDED_V3D_PROJECT_H

#include <string>
#include <vector>

#include <libv3dcommand/Tool.h>

#include "Scene.h"

namespace v3D
{

	/**
		A Project class.
		The Project is the root data structure. It maps to a file format
		and contains all of the scene data and other project-wide settings.
		Only a single instance can ever exist.
	 */
	class Project
	{
		public:
			/**
				get the global Project.
				@return the address of the global Project instance.
			 */
			static Project & instance(void);
			/**
				get the name of the Project.
				@return the name of the Project.
			 */
			std::string name(void) const;
			/**
				set the name of the Project.
				@param name the new name of the Project.
			 */
			void name(const std::string & name);
			/**
				get the currently active scene.
				@return a pointer to the active scene.
			*/
			ScenePtr activeScene(void) const;
			/**
				get the array of scenes in the project.
				@return the address of the project's scene array.
			*/
			std::vector<ScenePtr> & scenes(void);
			/**
				set the active scene.
				@param pointer to new active scene.
			 */
			void activeScene(ScenePtr scene);
			void addScene(ScenePtr scene);
			void clear(void);

			DAG::Transform * _selection;

			void activateTool(Tool * tool);
			void deactivateTool(void);
			Tool * activeTool(void) const;

		protected:
			Project();
			~Project();

		private:
			std::string				_name;
			std::vector<ScenePtr>	_scenes;
			ScenePtr				_activeScene;
			Tool * 					_activeTool;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_PROJECT_H
