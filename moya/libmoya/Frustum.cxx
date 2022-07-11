/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Frustum.h"

using namespace v3d::moya;

Frustum::Frustum()
{
}

Frustum::Frustum(const glm::mat4x4 & projection)
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

		[  0,  1,  2,  3 ]
		[  4,  5,  6,  7 ]
		[  8,  9, 10,  11 ]
		[ 12, 13, 14,  15 ]

		glm (column-major ordering):
		[  0,  4,  8,  12 ]
		[  1,  5,  9,  13 ]
		[  2,  6, 10,  14 ]
		[  3,  7, 11,  15 ]
*/
void Frustum::extract(const glm::mat4x4 & projection)
{
	Plane plane;
	// left clipping plane
	plane[0] = projection[3][0] + projection[0][0];
	plane[1] = projection[3][1] + projection[0][1];
	plane[2] = projection[3][2] + projection[0][2];
	plane[3] = projection[3][3] + projection[0][3];

	_clippingPlanes["left"] = plane;

	// right clipping plane
	plane[0] = projection[3][0] - projection[0][0];
	plane[1] = projection[3][1] - projection[0][1];
	plane[2] = projection[3][2] - projection[0][2];
	plane[3] = projection[3][3] - projection[0][3];

	_clippingPlanes["right"] = plane;

	// top clipping plane
	plane[0] = projection[3][0] - projection[1][0];
	plane[1] = projection[3][1] - projection[1][1];
	plane[2] = projection[3][2] - projection[1][2];
	plane[3] = projection[3][3] - projection[1][3];
	_clippingPlanes["top"] = plane;

	// bottom plane
	plane[0] = projection[3][0] + projection[1][0];
	plane[1] = projection[3][1] + projection[1][1];
	plane[2] = projection[3][2] + projection[1][2];
	plane[3] = projection[3][3] + projection[1][3];
	_clippingPlanes["bottom"] = plane;

	// near
	plane[0] = projection[2][0];
	plane[1] = projection[2][1];
	plane[2] = projection[2][2];
	plane[3] = projection[2][3];
	_clippingPlanes["near"] = plane;
	
	// far
	plane[0] = projection[3][0] - projection[2][0];
	plane[1] = projection[3][1] - projection[2][1];
	plane[2] = projection[3][2] - projection[2][2];
	plane[3] = projection[3][3] - projection[2][3];
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
int Frustum::intersect(const v3d::type::AABBox & aabb)
{
	size_t inside = _clippingPlanes.size();
	int hit = 0;
	// do plane/box intersection tests
	std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
	for (; it != _clippingPlanes.end(); it++)
	{
		hit = (it->second).classify(aabb);
		if (hit == Plane::OUTSIDE)
		{
			inside--;
		}
	}
	if (inside == _clippingPlanes.size())
	{
		return INSIDE;
	}
	if (inside == 0)
	{
		return OUTSIDE;
	}
	return CROSSING;
}

// clip a polygon against each of the clipping planes in the frustum
void Frustum::clip(boost::shared_ptr<Polygon> poly)
{
	std::map<std::string, Plane>::iterator it = _clippingPlanes.begin();
	for (; it != _clippingPlanes.end(); it++)
	{
		(it->second).clip(poly);
	}
}
