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
#ifndef INCLUDED_V3D_SCENE_H
#define INCLUDED_V3D_SCENE_H

#include <string>

#include "brep/HalfEdgeBRep.h"

namespace v3D
{

	/**
		The Scene class.
		The scene is the root of a scene graph.
		A scene stores
			geometry (meshes, curves ...)
			cameras
			lights
			groups
	*/
	class Scene
	{
		public:
			Scene();
			~Scene();

			/**
				Get the name of the scene.

				@return the name of the Scene.
			 */
			std::string		name(void) const;
			/**
				Set the name of the scene.

				@param name the name of the scene.
			 */
			void			name(const std::string & name);
			/**
				Get the bounds of the scene.

				@return the bounds of the scene.
			 */
			AABBox			bound(void) const;

			std::vector<MeshPtr> & meshes(void);

			void 			select(MeshPtr mesh);

			bool			active(void) const;
			void			active(bool sel);

		private:
			std::string	_name;
			bool		_active;

			std::vector<MeshPtr>	_meshes;
	};

	typedef Scene * ScenePtr;

}; // end namespace v3D

#endif // INCLUDED_V3D_SCENE_H
