/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Plane.h"

#include <cmath>
#include <cassert>

using namespace v3d::moya;


Plane::Plane()
{
}

Plane::Plane(const glm::vec3 & A, const glm::vec3 & B, const glm::vec3 & C)
{
	calculate(A, B, C);
}

Plane::~Plane()
{
}

void Plane::normalize(void)
{
	float mag;
	mag = std::sqrt(equation_[0] * equation_[0] + equation_[1] * equation_[1] + equation_[2] * equation_[2]);
	equation_[0] /= mag;
	equation_[1] /= mag;
	equation_[2] /= mag;
	equation_[3] /= mag;
}

void Plane::set(const glm::vec3 & n, float d)
{
	normal_ = n;
	distance_ = d;
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
void Plane::calculate(const glm::vec3 & A, const glm::vec3 & B, const glm::vec3 & C)
{
	normal_ = glm::normalize(glm::cross((B - A), (C - A))); // normal = AB*AC
	distance_ = glm::dot(normal_, A);
}

// tested - seems ok
void Plane::calculate(const glm::vec3 & normal, const glm::vec3 & point)
{
	normal_ = normal;
	distance_ = glm::dot(normal_, point);
	equation_[0] = normal_[0];
	equation_[1] = normal_[1];
	equation_[2] = normal_[2];
	equation_[3] = -(normal_[0] * point[0] + normal_[1] * point[1] + normal_[2] * point[2]);
}

float Plane::distance(const glm::vec3 & point) const
{
	return equation_[0] * point[0] + equation_[1] * point[1] + equation_[2] * point[2] + equation_[3];
//	return ((normal_ * point) - distance_);
}

// tested - seems ok
// classifies whether a point is on either side of the plane or on the plane itself.
int Plane::classify(const glm::vec3 & point) const
{
//	return !((normal_ * point) > distance_);
//	float d = equation_[A] * point.x() + equation_[B] * point.y() + equation_[C] * point.z() + equation_[D];
	
//	float d = distance(point);
	//d = _plane[0] * point[0] + _plane[1] * point[2] + _plane[2] * point[2] + _plane[3];
	/*
	Ax + By + Cz + D = 0
	Ax + By + Cz = -D
	*/
	/*
	float numer = (normal_ * point) + distance_;
	float denom = (normal_ * 
	*/
	float dist;
	//dist = -(normal_[0] * point[0] + normal_[1] * point[1] + normal_[2] * point[2]);
	//dist = (normal_ * point) - distance_;
	
	dist = (equation_[0] * point[0] + equation_[1] * point[1] + equation_[2] * point[2] + equation_[3]);

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
int Plane::classify(const v3d::type::AABBox & aabb) const
{
	glm::vec3 corners[8];
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
	return equation_[i];
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
	glm::vec3 hit;
	size_t nverts;
	 
	nverts = poly->vertexCount();
	s = poly->vertex(nverts - 1); // start with last vertex
	for (size_t j = 0; j < nverts; j++)
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
bool Plane::intersect(const glm::vec3 & start, const glm::vec3 & direction, glm::vec3 & hitPoint) const
{
	float denom = glm::dot(normal_, direction);
	if (denom == 0.0) // ray and plane are parallel
	{
		return false;
	}
	float tval = (distance_ - (glm::dot(normal_, start))) / denom;
	if (tval >= 0.0) // intersection isn't behind ray
	{
		hitPoint = start + direction * tval;
		return true;
	}
	return false;
}

// tested - seems ok
bool Plane::intersectEdge(const glm::vec3 & A, const glm::vec3 & B, glm::vec3 & hitPoint) const
{
	glm::vec3 direction = B - A;
	float denom = glm::dot(normal_, direction);
	if (denom == 0.0)
	{
		return false;
	}
	float tval = (distance_ - (glm::dot(normal_, A))) / denom;
	if (tval >= 0.0 && tval <= 1.0)
	{
		hitPoint = A + (direction * tval);
		return true;
	}
	return false;
}
