#include "Plane.h"

#include <cmath>
#include <cassert>

using namespace v3D;
using namespace v3D::Moya;

Plane::Plane()
{
}

Plane::Plane(const Vector3 & A, const Vector3 & B, const Vector3 & C)
{
	calculate(A, B, C);
}

Plane::~Plane()
{
}

void Plane::normalize(void)
{
	float mag;
	mag = std::sqrt(_equation[0] * _equation[0] + _equation[1] * _equation[1] + _equation[2] * _equation[2]);
	_equation[0] /= mag;
	_equation[1] /= mag;
	_equation[2] /= mag;
	_equation[3] /= mag;
}

void Plane::set(const Vector3 & n, float d)
{
	_normal = n;
	_distance = d;
}

/*
	calculate the normal and distance of a plane given three coplanar points
	normal N is the cross product of vectors U and V:
		U = B - A
		V = C - A
		N.x = (U.y * V.z) - (U.z * V.y)
		N.y = (U.z * V.x) - (U.x * V.z)
		N.z = (U.x * V.y) - (U.y * V.x)
	the resulting normal will not be in normalized form
	distance d is the dot product of normalized normal N and point A:
		d = (N.x * A.x) + (N.y * A.y) + (N.z * A.z)
	plane equation:
		Ax + By + Cz = d where x, y, and z are points in 3d space, A, B, and C are
			are the x, y, and z components of the surface normal, and d is the
			distance value. any point (x,y,z) that satisfies this equation lies on
			the plane
*/
void Plane::calculate(const Vector3 & A, const Vector3 & B, const Vector3 & C)
{
	_normal = (B - A).cross(C - A); // normal = AB*AC
	_normal.normalize();
	_distance = _normal * A;
}

// tested - seems ok
void Plane::calculate(const Vector3 & normal, const Vector3 & point)
{
	_normal = normal;
	_distance = (_normal * point);
	_equation[0] = _normal[0];
	_equation[1] = _normal[1];
	_equation[2] = _normal[2];
	_equation[3] = -(_normal[0] * point[0] + _normal[1] * point[1] + _normal[2] * point[2]);
}

float Plane::distance(const Vector3 & point) const
{
	return _equation[0] * point[0] + _equation[1] * point[1] + _equation[2] * point[2] + _equation[3];
//	return ((_normal * point) - _distance);
}

// tested - seems ok
// classifies whether a point is on either side of the plane or on the plane itself.
int Plane::classify(const Vector3 & point) const
{
//	return !((_normal * point) > _distance);
//	float d = _equation[A] * point.x() + _equation[B] * point.y() + _equation[C] * point.z() + _equation[D];
	
//	float d = distance(point);
	//d = _plane[0] * point[0] + _plane[1] * point[2] + _plane[2] * point[2] + _plane[3];
	/*
	Ax + By + Cz + D = 0
	Ax + By + Cz = -D
	*/
	/*
	float numer = (_normal * point) + _distance;
	float denom = (_normal * 
	*/
	float dist;
	//dist = -(_normal[0] * point[0] + _normal[1] * point[1] + _normal[2] * point[2]);
	//dist = (_normal * point) - _distance;
	
	dist = (_equation[0] * point[0] + _equation[1] * point[1] + _equation[2] * point[2] + _equation[3]);

	if (dist < 0.0)
	{
		return NEGATIVE;
	}
	if (dist > 0.0)
	{
		return POSITIVE;
	}
	return ON_PLANE;
}

/*
	classify the 8 points of the aabb against the plane
	see: http://www.flipcode.com/articles/article_frustumculling.shtml
*/
int Plane::classify(const AABBox & aabb) const
{
	Vector3 corners[8];
	aabb.vertices(corners);
	int inside = 8; // start with all 8 points inside
	for (unsigned int i = 0; i < 8; i++)
	{
		int side = classify(corners[i]);
		if (side == NEGATIVE)
		{
			inside--;
		}
	}
	if (inside == 0) // all points were outside
	{
		return OUTSIDE;
	}
	if (inside == 8) // all points were inside
	{
		return INSIDE;
	}
	// points were on both sides so box intersects plane
	return CROSSING;
}

float & Plane::operator [] (unsigned int i)
{
	assert(i < 4);
	return _equation[i];
}


/* note - clipping happens after culling

	clip a single polygon to a single plane
	this is a 3D Sutherland-Hodgman Polygon Clipper 
	instead of clipping against a single clipping rectangle edge, we clip against
	a plane.
*/
void Plane::clip(boost::shared_ptr<Polygon> poly)
{
	boost::shared_ptr<Polygon> clippedPoly(new Polygon);
	Vertex s, p, i;
	Vector3 hit;
	unsigned int nverts;
	 
	nverts = poly->vertexCount();
	s = poly->vertex(nverts - 1); // start with last vertex
	for (unsigned int j = 0; j < nverts; j++)
	{
		p = poly->vertex(j);
		/*
		 there are 4 possible test cases:
		 	case 1: s & p both inside	 - in/in
		 	case 2: s inside, p outside - in/out
		 	case 3: s & p both outside	 - out/out
		 	case 4: s outside, p inside - out/in
		 */
		int side = classify(p.point());
		if (side == POSITIVE || side == ON_PLANE) // cases 1 & 4
		{
			 side = classify(s.point());
			 if (side == POSITIVE || side == ON_PLANE)
			 {
				  // case 1
				  clippedPoly->addVertex(p);
			 }
			 else
			 {
				  // case 4
				  intersectEdge(s.point(), p.point(), hit);
				  i.point(hit);
				  clippedPoly->addVertex(i);
				  clippedPoly->addVertex(p);
			 }
		}
		else // cases 2 & 3
		{
			 side = classify(s.point());
			 if (side == POSITIVE || side == ON_PLANE)
			 {
				  // case 2
				  intersectEdge(s.point(), p.point(), hit);
				  i.point(hit);
				  clippedPoly->addVertex(i);
			 }
			 else
			 {
				  // case 3
				  // entire edge is clipped - no action required
			 }
		}
		s = p;
	}
	poly = clippedPoly;
}

// ray intersection test
bool Plane::intersect(const Vector3 & start, const Vector3 & direction, Vector3 & hitPoint) const
{
	float denom = _normal * direction;
	if (denom == 0.0) // ray and plane are parallel
	{
		return false;
	}
	float tval = (_distance - (_normal * start)) / denom;
	if (tval >= 0.0) // intersection isn't behind ray
	{
		hitPoint = start + direction * tval;
		return true;
	}
	return false;
}

// tested - seems ok
bool Plane::intersectEdge(const Vector3 & A, const Vector3 & B, Vector3 & hitPoint) const
{
	Vector3 direction = B - A;
	float denom = _normal * direction; // dot product
	if (denom == 0.0)
	{
		return false;
	}
	float tval = (_distance - (_normal*A)) / denom;
	if (tval >= 0.0 && tval <= 1.0)
	{
		hitPoint = A + (direction * tval);
		return true;
	}
	return false;
}
