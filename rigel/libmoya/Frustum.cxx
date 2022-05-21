/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include "Frustum.h"

using namespace v3D::Moya;

Frustum::Frustum()
{
}

Frustum::Frustum(const Matrix4 & projection)
{
	extract(projection);
}

Frustum::~Frustum()
{
}

/*
	[ a, b, c, d ]	[  1,  2,  3,  4 ]	[ 11, 12, 13, 14 ]
	[ e, f, g, h ]	[  5,  6,  7,  8 ]	[ 21, 22, 23, 24 ]
	[ i, j, k, l ]	[  9, 10, 11, 12 ]	[ 31, 32, 33, 34 ]
	[ m, n, o, p ]	[ 13, 14, 15, 16 ]	[ 41, 42, 43, 44 ]

	the plane extraction is described in: http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
*/
void Frustum::extract(const Matrix4 & projection)
{
	Plane plane;
	// left clipping plane
	plane[0] = projection[3] + projection[0];
	plane[1] = projection[7] + projection[4];
	plane[2] = projection[11] + projection[8];
	plane[3] = projection[15] + projection[12];
	_clippingPlanes["left"] = plane;

	// right clipping plane
	plane[0] = projection[3] - projection[0];
	plane[1] = projection[7] - projection[4];
	plane[2] = projection[11] - projection[8];
	plane[3] = projection[15] - projection[12];
	_clippingPlanes["right"] = plane;

	// top clipping plane
	plane[0] = projection[3] - projection[1];
	plane[1] = projection[7] - projection[5];
	plane[2] = projection[11] - projection[9];
	plane[3] = projection[15] - projection[13];
	_clippingPlanes["top"] = plane;

	// bottom plane
	plane[0] = projection[3] + projection[1];
	plane[1] = projection[7] + projection[5];
	plane[2] = projection[11] + projection[9];
	plane[3] = projection[15] + projection[13];
	_clippingPlanes["bottom"] = plane;

	// near
	plane[0] = projection[2];
	plane[1] = projection[6];
	plane[2] = projection[10];
	plane[3] = projection[14];
	_clippingPlanes["near"] = plane;
	
	// far
	plane[0] = projection[3] - projection[2];
	plane[1] = projection[7] - projection[6];
	plane[2] = projection[11] - projection[10];
	plane[3] = projection[15] - projection[14];
	_clippingPlanes["far"] = plane;
}

void Frustum::normalize(void)
{
	std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
	for (; it != _clippingPlanes.end(); it++)
	{
		(it->second).normalize();
	}
}

/*
	tests aabb against all of the frustum's clipping planes 
	aabb is either inside, outside, or intersecting the frustum
*/
int Frustum::intersect(const AABBox & aabb)
{
	unsigned int inside = _clippingPlanes.size();
	int hit = 0;
	// do plane/box intersection tests
	std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
	for (; it != _clippingPlanes.end(); it++)
	{
		hit = (it->second).classify(aabb);
		if (hit == Plane::OUTSIDE)
			inside--;
	}
	if (inside == _clippingPlanes.size())
		return INSIDE;
	if (inside == 0)
		return OUTSIDE;
	return CROSSING;
}

// clip a polygon against each of the clipping planes in the frustum
void Frustum::clip(PolygonPtr & poly)
{
	std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
	for (; it != _clippingPlanes.end(); it++)
	{
		(it->second).clip(poly);
	}
}
