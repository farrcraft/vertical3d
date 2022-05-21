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

#include <iostream>
#include <cmath>

#include "CreatePolyCommandSet.h"

#include <libv3dcommand/CommandDirectory.h>
#include <libv3dcore/Project.h>

#include "../Window.h"

using namespace v3D;

CreatePolyCommandSet::CreatePolyCommandSet()
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "create::poly::plane");
	dir.connect(this, "create::poly::cube");
	dir.connect(this, "create::poly::cone");
	dir.connect(this, "create::poly::cylinder");
}

CreatePolyCommandSet::~CreatePolyCommandSet()
{
}

bool CreatePolyCommandSet::exec(const std::string & cmd)
{
	if (cmd == "create::poly::cube")
	{
		cube();
	}
	else if (cmd == "create::poly::cylinder")
	{
		cylinder();
	}
	else if (cmd == "create::poly::cone")
	{
		cone();
	}
	else if (cmd == "create::poly::plane")
	{
		plane();
	}
	else
	{
		return false;
	}

	Window::instance()->redraw();

	return true;
}

void CreatePolyCommandSet::plane(void)
{
	float length = 1.0;
	float width  = 1.0;
	length /= 2.0;
	width  /= 2.0;
	MeshPtr mesh(new Mesh());

	Vector3 norm(0.0, 1.0, 0.0);

	// add 4 vertices
	std::vector<Vector3> vertices;
	vertices.push_back(Vector3(-width, 0.0, -length));
	vertices.push_back(Vector3( width, 0.0, -length));
	vertices.push_back(Vector3( width, 0.0,  length));
	vertices.push_back(Vector3(-width, 0.0,  length));
	mesh->addFace(vertices, norm);

	// plane is constructed
	// add to scene
	ScenePtr scene = Project::instance().activeScene();
	scene->meshes().push_back(mesh);

	// set newly created object as selected object
	scene->select(mesh);
	Project::instance()._selection = mesh;
}

void CreatePolyCommandSet::cone(void)
{
	int sides = 8;
	float height = 1.0;
	float radius = 0.5;

	float delta = 2.0 * PI / sides;

	MeshPtr mesh(new Mesh());

	// create points in cone
	std::vector<Vector3> points;
	for (int k = 0; k <= sides; k++)
	{
		Vector3 p;
		int v;
		p[0] = cos(delta * k) * radius;
		p[1] = sin(delta * k) * radius;
		points.push_back(p);
	}
	// top point
	Vector3 v3(0.0, 0.0, height);
	points.push_back(v3);

	// create polygons in cone
	for (int k = 1; k <= sides; k++)
	{
		Vector3 norm(1.0, 0.0, 0.0);
		std::vector<Vector3> face;
		Vector3 v1, v2;
		v1 = points[k-1];
		if (k == points.size())
			v2 = points[0];
		else
			v2 = points[k];

		// add vertices to polygon
		face.push_back(v1);
		face.push_back(v2);
		face.push_back(v3);

		// add polygon to mesh
		mesh->addFace(face, norm);
	}

	// add to scene
	ScenePtr scene = Project::instance().activeScene();
	scene->meshes().push_back(mesh);

	// set newly created object as selected object
	scene->select(mesh);
	Project::instance()._selection = mesh;
}

void CreatePolyCommandSet::cylinder(void)
{
	int sides = 8;
	float height = 1.0;
	float radius = 0.5;

	float delta = 2.0 * PI / sides;

	MeshPtr mesh(new Mesh());

	// create points in cylinder
	std::vector<Vector3> points;
	std::vector<Vector3> points_top;
	for (int k = 0; k <= sides; k++)
	{
		Vector3 p;
		p[0] = cos(delta * k) * radius;
		p[1] = sin(delta * k) * radius;
		p[2] = 0.0;
		points.push_back(p);
		// corresponding top point
		p[2] += height;
		points_top.push_back(p);
	}

	// create each polygon side of the cylinder
	for (int k = 1; k <= sides; k++)
	{
		std::vector<Vector3> face;
		Vector3 v1, v2, v3, v4;
		Vector3 norm(1.0, 0.0, 0.0);

		// get vertex indices
		v1 = points[k - 1];
		if (k == points.size())
			v2 = points[0];
		else
			v2 = points[k];
		// top two points (z + height)
		if (k == points_top.size())
			v3 = points_top[0];
		else
			v3 = points_top[k];
		v4 = points_top[k - 1];

		// add vertices to polygon
		face.push_back(v1);
		face.push_back(v2);
		face.push_back(v3);
		face.push_back(v4);

		// add polygon to mesh
		mesh->addFace(face, norm);
	}

	// add to scene
	ScenePtr scene = Project::instance().activeScene();
	scene->meshes().push_back(mesh);

	// set newly created object as selected object
	scene->select(mesh);
	Project::instance()._selection = mesh;
}

void CreatePolyCommandSet::cube (void)
{
	// this should probably go in PolyCubeCommand::exec() or similar
	float length = 1.0;
	float height = 1.0;
	float width  = 1.0;
	length /= 2.0;
	height /= 2.0;
	width  /= 2.0;
	MeshPtr mesh(new Mesh());

	// start new polygon - front
	std::vector<Vector3> points;
	Vector3 norm(0.0, 0.0, -1.0);
	// add 4 vertices
	points.push_back(Vector3(-width, -height, -length));
	points.push_back(Vector3( width, -height, -length));
	points.push_back(Vector3( width,  height, -length));
	points.push_back(Vector3(-width,  height, -length));
	mesh->addFace(points, norm);

	// next polygon - right
	points.clear();
	norm = Vector3(1.0, 0.0, 0.0);
	points.push_back(Vector3(width,  -height, -length));
	points.push_back(Vector3( width, -height,  length));
	points.push_back(Vector3( width,  height,  length));
	points.push_back(Vector3( width,  height, -length));
	mesh->addFace(points, norm);

	// next polygon - top
	points.clear();
	norm = Vector3(0.0, 1.0, 0.0);
	points.push_back(Vector3(-width,  height, -length));
	points.push_back(Vector3( width,  height, -length));
	points.push_back(Vector3( width,  height,  length));
	points.push_back(Vector3(-width,  height,  length));
	mesh->addFace(points, norm);

	// next polygon - left
	points.clear();
	norm = Vector3(-1.0, 0.0, 0.0);
	points.push_back(Vector3(-width, -height,  length));
	points.push_back(Vector3(-width, -height, -length));
	points.push_back(Vector3(-width,  height, -length));
	points.push_back(Vector3(-width,  height,  length));
	mesh->addFace(points, norm);

	// next polygon - back
	points.clear();
	norm = Vector3(0.0, 0.0, 1.0);
	points.push_back(Vector3( width, -height,  length));
	points.push_back(Vector3(-width, -height,  length));
	points.push_back(Vector3(-width,  height,  length));
	points.push_back(Vector3( width,  height,  length));
	mesh->addFace(points, norm);

	// next polygon - bottom
	points.clear();
	norm = Vector3(0.0, -1.0, 0.0);
	points.push_back(Vector3( width, -height, -length));
	points.push_back(Vector3(-width, -height, -length));
	points.push_back(Vector3(-width, -height,  length));
	points.push_back(Vector3( width, -height,  length));
	mesh->addFace(points, norm);

	// cube is constructed
	// add to scene
	ScenePtr scene = Project::instance().activeScene();
	scene->meshes().push_back(mesh);

	// set newly created object as selected object
	scene->select(mesh);
	Project::instance()._selection = mesh;

	std::cerr << "added cube" << std::endl;
}
