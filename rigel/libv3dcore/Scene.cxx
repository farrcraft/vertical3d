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
#include "Scene.h"

using namespace v3D;

Scene::Scene()
{
}

Scene::~Scene()
{
}

bool Scene::active(void) const
{
	return _active;
}

void Scene::active(bool sel)
{
	_active = true;
}

std::vector<MeshPtr> & Scene::meshes(void)
{
	return _meshes;
}

std::string Scene::name(void) const
{
	return _name;
}

void Scene::name(const std::string & name)
{
	_name = name;
}

void Scene::select(MeshPtr mesh)
{
	for (unsigned int index = 0; index  < _meshes.size(); index++)
	{
		_meshes[index]->selected(false);
	}
	mesh->selected(true);
}

AABBox Scene::bound(void) const
{
	AABBox extents;
	if (_meshes.size() == 0)
		return extents;
	AABBox object_bound;
	Vector3 min, max, object_min, object_max;
	object_bound = _meshes[0]->bound();
	min = _meshes[0]->matrix() * object_bound.min();
	max = _meshes[0]->matrix() * object_bound.max();

	for (unsigned int index = 1; index  < _meshes.size(); index++)
	{
		object_bound = _meshes[index]->bound();
		/*  object_bound is in object space
			multiply by object's transform to get to world space
			this may be flawed when dealing with nested group objects
			there might be other parent transforms to apply before we're actually in world space
		*/
		object_min = _meshes[index]->matrix() * object_bound.min();
		object_max = _meshes[index]->matrix() * object_bound.max();

		if (object_min[0] < min[0])
			min[0] = object_min[0];
		if (object_min[1] < min[1])
			min[1] = object_min[1];
		if (object_min[2] < min[2])
			min[2] = object_min[2];

		if (object_max[0] > max[0])
			max[0] = object_max[0];
		if (object_max[1] > max[1])
			max[1] = object_max[1];
		if (object_max[2] > max[2])
			max[2] = object_max[2];
	}

	extents.extents(min, max);
	return extents;
}
