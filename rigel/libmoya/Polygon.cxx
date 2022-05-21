/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>
#include <cassert>
#include <cmath>

#include "Polygon.h"
#include "Plane.h"
#include "Moya.h"

using namespace v3D;
using namespace v3D::Moya;


Polygon::Polygon()
{
}

Polygon::~Polygon()
{
}

void Polygon::addVertex(Vertex vert)
{
	_vertices.push_back(vert);
}

unsigned int Polygon::vertexCount(void) const
{
	return _vertices.size();
}

Vertex Polygon::vertex(unsigned int idx) const
{
	assert(idx < _vertices.size());
	return _vertices[idx];
}

void Polygon::removeVertex(unsigned int idx)
{
}

Vertex & Polygon::operator [] (unsigned int idx)
{
	assert(idx < _vertices.size());
	return _vertices[idx];
}

// return an object space bound of the polygon
AABBox Polygon::bound(void) const
{
	AABBox bound;

	if (_vertices.size() == 0)
		return bound;
	/*
		move over each vertex and record min/max
	*/
	std::vector<Vertex>::const_iterator it = _vertices.begin();
	Vector3 min, max;
	min = _vertices[0].point();
	max = min;
	std::cerr << "poly bounds based on - ";
	for (; it != _vertices.end(); it++)
	{
		std::cerr << " v = " << (*it).point();
		if (min[0] > (*it).point()[0])
		{
			min[0] = (*it).point()[0];
		}
		if (max[0] < (*it).point()[0])
		{
			max[0] = (*it).point()[0];
		}

		if (min[1] > (*it).point()[1])
		{
			min[1] = (*it).point()[1];
		}
		if (max[1] < (*it).point()[1])
		{
			max[1] = (*it).point()[1];
		}

		if (min[2] > (*it).point()[2])
		{
			min[2] = (*it).point()[2];
		}
		if (max[2] < (*it).point()[2])
		{
			max[2] = (*it).point()[2];
		}
	}
	std::cerr << std::endl;
	bound.extents(min, max);

	return bound;
}

/* split the polygon into smaller polygons
   how do we decide where to split polygons?
   how many smaller polygons do we make?

   recursive subdivision seems like the way to do it.
   it would work similar to frustum clipping.
   a polygon in plane A will be divided into two halves by a plane B
   that is perpendicular to plane A.

   the generated sub polygons should be fed back to the active render context.

   each call to split should divide the polygon into 4 smaller pieces.

	there are two ways to go on this:
		- splitting using polygon/plane intersection
		- polygon triangulation

	the problem with triangulation is that the next step is dicing where we'll
	want to convert the triangles into a grid of equilateral micropolygons.

	using plane intersection suffers from not having a specific plane to use for
	the intersection. there's also no guarantee that the resulting geometry will
	be any better than what we'd get with triangulation.

	first, we'll try implementing plane intersection (since it should be easier
	than doing triangulation here). there are two ways to figure out what plane
	to use:
		- (poly.v1 - poly.v0).normalize().cross(poly.normal)
		- find rotation that maps poly.normal to [0.0, 0.0, 1.0]

	the first method uses an edge of the polygon and its normal to find a normal
	for the plane that is perpendicular to both.

	the second finds a matrix that can transform the polygon's vertices onto the
	x/y plane.

	the last piece we need is a point that lies on the plane. this should be the
	center of the polygon.
*/

/*
internal splitter - will be called 3 times by split()
args p1 and p2 should be pointers to empty polygons to store the resulting split 
polygons
*/
void Polygon::split(const Plane & plane, Polygon * p1, Polygon * p2)
{
	// intersect each edge with the plane
	Vector3 A, B, hit;
	int side;
	Vertex vert;
	int vcount;
	for (unsigned int i = 0; i < _vertices.size(); i++)
	{
		vcount = _vertices.size();
		A = _vertices[i].point();
		if (i == (vcount - 1))
			B = _vertices[0].point();
		else
			B = _vertices[i + 1].point();
		// classify which side of the plane A is on
		// ...
		side = plane.classify(A);
		if (plane.intersectEdge(A, B, hit))
		{
			std::cerr << "Polygon::split - plane/edge intersection at " << hit << " A = " << A << " B = " << B << std::endl;
			// A is on one side and B on the other
			// hit is the common vertex both new edges share
			// create a new edge for p1 and p2 e1 (A, hit), e2 (hit, B)
			vert.point(A);
			if (i == 0)
			{
				if (side < 0)
				{
					std::cerr << "Polygon::split - A in p1" << std::endl;
					p1->addVertex(vert);
				}
				else
				{
					std::cerr << "Polygon::split - A in p2" << std::endl;
					p2->addVertex(vert);
				}
			}

			vert.point(hit);
			p1->addVertex(vert);
			p2->addVertex(vert);

			vert.point(B);
			if (i != (vcount - 1))
			{
				if (side < 0)
				{
					std::cerr << "Polygon::split - B in p2" << std::endl;
					p2->addVertex(vert);
				}
				else
				{
					std::cerr << "Polygon::split - B in p1" << std::endl;
					p1->addVertex(vert);
				}
			}
		}
		else
		{
			std::cerr << "Polygon::split - no intersection at A = " << A << " B = " << B << std::endl;
			// add edge to poly corresponding to the classification of A
			// since there was no intersection, B will be on the same side
			vert.point(A);
			int bside = plane.classify(B);
			assert(side == bside);
			std::cerr << "Polygon::split - sides = " << side << " == " << bside << std::endl;
			if (side <= 0)
			{
				if (i == 0)
				{
					std::cerr << "Polygon::split - A in p1" << std::endl;
					p1->addVertex(vert);
				}
				vert.point(B);
				if (i != (vcount - 1))
				{
					std::cerr << "Polygon::split - B in p1" << std::endl;
					p1->addVertex(vert);
				}
			}
			else
			{
				if (i == 0)
				{
					std::cerr << "Polygon::split - A in p2" << std::endl;
					p2->addVertex(vert);
				}
				vert.point(B);
				if (i != (vcount - 1))
				{
					std::cerr << "Polygon::split - B in p2" << std::endl;
					p2->addVertex(vert);
				}
			}
		}
	}
}

void Polygon::split(void)
{
	std::cerr << "Polygon::split - splitting polygon." << std::endl;
	// calculate polygon's normal from the polygon's first two vertices
	Vector3 v0, v1, n;
	v0 = _vertices[0].point() - _vertices[1].point();
	v1 = _vertices[2].point() - _vertices[1].point();
	n = v1.cross(v0);
	n.normalize();
	// calculate plane's normal
	Vector3 pn;
	pn = n.cross(v0);
	pn.normalize();

	std::cerr << "Polygon::split - poly normal = " << n << " plane normal = " << pn << std::endl;

	// find a point on the plane
	AABBox bounds;
	Vector3 mp, pop;
	bounds = bound();
	mp = (bounds.max() - bounds.min()) / 2.0;
	pop = bounds.min() + mp;
	std::cerr << "Polygon::split - midpoint = " << mp << " point on plane = " << pop << std::endl;

	// create the intersection plane from pop and pn
	Plane plane;
	plane.calculate(pn, pop);

	// two new (potentially) polygons created as a result of splitting
	PolygonPtr p1, p2;
	p1 = new Polygon();
	p2 = new Polygon();

	// first split
	split(plane, p1, p2);
	
	// calculate normal for 2nd plane to split against
	pn = n.cross(pn);
	pn.normalize();

	std::cerr << "Polygon::split - plane normal 2 = " << pn << std::endl;

	// create the new intersection plane
	plane.calculate(pn, pop);

	// split the two new polygons against the two created by the last split
	PolygonPtr pf1, pf2, pf3, pf4;
	pf1 = new Polygon();
	pf2 = new Polygon();
	pf3 = new Polygon();
	pf4 = new Polygon();
	p1->split(plane, pf1, pf2);
	p2->split(plane, pf3, pf4);

	// free up the two intermediate polygons
	delete p1;
	delete p2;	

	std::cerr << "Polygon::split - p1 vcount = " << pf1->vertexCount() << " p2 vcount = " << pf2->vertexCount() << std::endl;
	std::cerr << "Polygon::split - p3 vcount = " << pf3->vertexCount() << " p4 vcount = " << pf4->vertexCount() << std::endl;
	// all that remains is to feed the new polygons p1 & p2 back into the top of 
	// the renderer and discard the old one. we also must make sure not to put
	// empty polygons back in.
	if (pf1->vertexCount() > 0)
	{
		if (pf1->vertexCount() <= 2)
			std::cerr << "Polygon::split - generated polygon p1 has < 3 vertices." << std::endl;
		std::cerr << "Polygon::split - adding p1 to RC" << std::endl;
		RenderEngine::instance().activeRenderContext().addPolygon(pf1);
	}
	else
	{
		std::cerr << "Polygon::split - empty polygon p1 discarded." << std::endl;
		delete pf1;
	}
	if (pf2->vertexCount() > 0)
	{
		if (pf2->vertexCount() <= 2)
			std::cerr << "Polygon::split - generated polygon p2 has < 3 vertices." << std::endl;
		std::cerr << "Polygon::split - adding p2 to RC" << std::endl;
		RenderEngine::instance().activeRenderContext().addPolygon(pf2);
	}
	else
	{
		std::cerr << "Polygon::split - empty polygon p2 discarded." << std::endl;
		delete pf2;
	}
	if (pf3->vertexCount() > 0)
	{
		if (pf3->vertexCount() <= 2)
			std::cerr << "Polygon::split - generated polygon p3 has < 3 vertices." << std::endl;
		std::cerr << "Polygon::split - adding p3 to RC" << std::endl;
		RenderEngine::instance().activeRenderContext().addPolygon(pf3);
	}
	else
	{
		std::cerr << "Polygon::split - empty polygon p3 discarded." << std::endl;
		delete pf3;
	}
	if (pf4->vertexCount() > 0)
	{
		if (pf4->vertexCount() <= 2)
			std::cerr << "Polygon::split - generated polygon p4 has < 3 vertices." << std::endl;
		std::cerr << "Polygon::split - adding p4 to RC" << std::endl;
		RenderEngine::instance().activeRenderContext().addPolygon(pf4);
	}
	else
	{
		std::cerr << "Polygon::split - empty polygon p4 discarded." << std::endl;
		delete pf4;
	}
}

/*
	dice - turn the polygon into a grid of micropolygons
	dicing is done in eye space
	this means we can work in x, y space and not worry about z
*/
bool Polygon::dice(MicroPolygonGridPtr & grid)
{
	std::cerr << "Polygon::dice - dicing polygon into micropolygon grids." << std::endl;

	/*
		the poly should already be in (unprojected) eye space
		we need to know how big our grid is
		micropolygons will always be approximately 1/2 pixel per side in size
		grids can be any size up to a maximum size
		ideally the maximum grid size should be no larger than the bucket size
		1 pixel = 4 micropolygons
		the gridsize is stored as the number of micropolygons in the grid
		does shading rate affect micropolygon size?
		larger shading rate = coarser dicing / smaller = finer
		grids may span multiple buckets
		diceable polygons should already be split down to match the grid size
	 */
	// grid dimensions are sizeXsize
	unsigned int size = static_cast<unsigned int>(sqrt(RenderEngine::instance().gridSize()));
	// the eye space bound of the poly
	AABBox bounds = bound();	
	Vector3 bound_min = bounds.min();
	Vector3 bound_max = bounds.max();
	// get the size of each micropoly
	float offset_x = (bound_max[0] - bound_min[0]) / size;
	float offset_y = (bound_max[1] - bound_min[1]) / size;
	/*
		convert the eye space bound to screen space
	*/
	Matrix4 screen = RenderEngine::instance().activeRenderContext().coordinateSystem("screen");
	screen *= RenderEngine::instance().activeRenderContext().coordinateSystem("raster");
	bound_min = screen * bound_min;
	bound_max = screen * bound_max;

	// screen transform might've flipped some components of min & max
	if (bound_min[0] > bound_max[0])
		swap(bound_min[0], bound_max[0]);
	if (bound_min[1] > bound_max[1])
		swap(bound_min[1], bound_max[1]);
	if (bound_min[2] > bound_max[2])
		swap(bound_min[2], bound_max[2]);

	bounds.extents(bound_min, bound_max);
	/*
		to build the grid do we:
			iterate the vertices of the poly
		or
			iterate the points on the grid

		we expect the input poly to be about the same size as a grid but what if 
		it is smaller or larger?

		first:
		use the first part of graham's scan algorithm:
		http://softsurfer.com/Archive/algorithm_0109/algorithm_0109.htm
		to arrange the vertices in order.
		once they're arranged, the initial vertices make up the starting grid
		then edges are subdivided and new edges are added until the edges
		match the maximum micropolygon size

		instead of starting out with a grid size of iXj we start out with a
		single "micro"polygon equivalent to the initial polygon

		micropolygons are quadrilaterals but the undiced polygons might have
		more than 4 vertices. how do we handle this?

		initialize a sizeXsize grid of booleans to false
		iterate over the poly's vertices
			put the vertex in the proper spot on the grid
			mark the corresponding boolean grid point true
		iterate over the boolean grid points that are still false
			create a new vertex iterpolated between the existing vertices
	*/
	std::vector<bool> empty;
	unsigned int idx = 0;
	for (; idx < (size * size); idx++)
		empty[idx] = false;

	for (idx = 0; idx < _vertices.size(); idx++)
	{
		
	}

	// don't continue dicing this polygon
	return false;
}
