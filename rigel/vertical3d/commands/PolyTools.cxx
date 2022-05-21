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

#include "PolyTools.h"

#include <libv3dcommand/CommandDirectory.h>
#include <libv3dcore/Project.h>

#include "../Window.h"

using namespace v3D;

SplitEdgeTool::SplitEdgeTool() : _draw(false)
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "transform::poly::split_edge");
}

SplitEdgeTool::~SplitEdgeTool()
{
}

bool SplitEdgeTool::exec(const std::string & cmd)
{
	if (cmd == "transform::poly::split_edge")
	{
		Project::instance().activateTool(this);
		// get selected mesh
		// while mouse button not clicked
			// draw vertex handle at nearest edge-mouse intersection point
		// insert new vertex
		// remove original edge
		// insert two new edges
	}
	else
	{
		return false;
	}
	return true;
}

bool SplitEdgeTool::motion(int x, int y)
{
	// get selected mesh
	Mesh * mesh = dynamic_cast<Mesh*>(Project::instance()._selection);
	// find nearest edge-mouse intersection point
	// FIXME: use geometric tools point-line math instead
	Vector2 p(x, y);
	Vector3 point;
	point = Window::instance()->activeView()->unproject(p);
	/*
		iterate through each edge of mesh and find edge nearest to unprojected point

		find the nearest intersection of edge and unprojected point
		if the unprojected point lies on the edge then we already have the point
		otherwise, use a subdivision method:
			find distance between point and each end of the edge
			split the edge
			find distance between point and split
			discard the furthest (greatest) of the 3 edge points (should always be an exterior point)
			if distance from 2 remaining edge points is greater than the splitting threshold
			continue splitting the edge
			else the edge point nearest the unprojected point is our final point
	*/
	// for each edge in mesh
/*
	std::vector<Mesh::Vertex> & vertices = mesh->vertices();
	std::vector<Mesh::Edge> & edges = mesh->edges();
	std::vector<Mesh::Edge>::iterator eit = edges.begin();
	unsigned int edge_count = 0;
*/
	unsigned int nearest_edge = 0;
	real_t nearest_distance = 0.0;
	real_t nearest_t;
	real_t t;
	real_t distance;
	bool first_edge = true;

	unsigned int edge_count = 0;
	for (; edge_count < mesh->edgeCount(); edge_count++)
	{
		HalfEdge * edge = mesh->edge(edge_count);

		Vector3 v1 = mesh->vertex(edge->vertex())->point();
		Vector3 v2 = mesh->vertex(mesh->edge(edge->next())->vertex())->point();

		// don't consider edges with a screen space length < 1.0
		Vector3 sv1 = Window::instance()->activeView()->project(v1);
		Vector3 sv2 = Window::instance()->activeView()->project(v2);
		real_t screen_length = sv1.distance(sv2);
		if (screen_length > 1.0)
		{
			//	find edge nearest point
			distance = pointLineSegmentDistanceSquared(point, v1, v2, t);

			// if this isn't the first edge processed in the mesh
			if (!first_edge)
			{
				if (distance < nearest_distance)
				{
					nearest_edge = edge_count;
					nearest_distance = distance;
					nearest_t = t;
				}
			}
			else
			{
				first_edge = false;
				nearest_distance = distance;
				nearest_t = t;
			}
		}
	}

	// find point on edge to use as new vertex
	// get edge endpoints
	Vector3 ep1 = mesh->vertex(mesh->edge(nearest_edge)->vertex())->point();
	Vector3 ep2 = mesh->vertex(mesh->edge(mesh->edge(nearest_edge)->next())->vertex())->point();
	_vertex = ep1 + (ep2 * nearest_t);

	// save edge
	_edge = nearest_edge;
	// set flag to draw selected point
	_draw = true;
}

bool SplitEdgeTool::button(unsigned int num, bool press, int x, int y)
{
	if (_draw)
	{
		Mesh * mesh = dynamic_cast<Mesh*>(Project::instance()._selection);
		mesh->splitEdge(_edge, _vertex);
	}
	_draw = false;
	Project::instance().deactivateTool();
}

bool SplitEdgeTool::draw(void)
{
	// draw vertex handle
	if (_draw)
	{
		glColor3f(0.7, 0.5, 0.4);

		Vector3 u = Window::instance()->activeView()->camera()->up();
		Vector3 v = Window::instance()->activeView()->camera()->right();
		// scale down the u & v direction vectors to use as a step value
		u *= 0.0125;
		v *= 0.0125;

		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);

		Vector3 mid = _vertex;
		Vector3 handle = mid - u - v;
		glBegin(GL_POLYGON);
		glVertex3f(handle[0], handle[1], handle[2]);
		handle = mid + u - v;
		glVertex3f(handle[0], handle[1], handle[2]);
		handle = mid + u + v;
		glVertex3f(handle[0], handle[1], handle[2]);
		handle = mid - u + v;
		glVertex3f(handle[0], handle[1], handle[2]);
		glEnd();
	}
}


SplitFaceTool::SplitFaceTool()
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "transform::poly::split_face");
}

SplitFaceTool::~SplitFaceTool()
{
}

bool SplitFaceTool::exec(const std::string & cmd)
{
	if (cmd == "transform::poly::split_face")
	{
		Project::instance().activateTool(this);
		Project::instance().deactivateTool();
	}
	else
	{
		return false;
	}
	return true;
}

ExtrudeFaceTool::ExtrudeFaceTool()
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "transform::poly::extrude_face");
}

ExtrudeFaceTool::~ExtrudeFaceTool()
{
}

bool ExtrudeFaceTool::exec(const std::string & cmd)
{
	if (cmd == "transform::poly::extrude_face")
	{
		Project::instance().activateTool(this);
		// get selected mesh
		// get selected face
		// duplicate face, vertices, edges
		// add new edges connecting original and duped vertices
		// add new faces from original and duped edges & new edges
		Project::instance().deactivateTool();
	}
	else
	{
		return false;
	}
	return true;
}
