/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <libv3dtypes/Vector3.h>
#include <libv3dtypes/AABBox.h>

#include "Polygon.h"

namespace v3D
{

	namespace Moya
	{

		/*
			defines a plane from one of:
				N dot P = D
				Ax+Bx+Cz = D
		*/
		class Plane
		{
			public:
				Plane();
				Plane(const Vector3 & A, const Vector3 & B, const Vector3 & C);
				~Plane();
	
				enum HalfSpace
				{
					NEGATIVE = -1,
					OUTSIDE = -1,
					ON_PLANE = 0,
					CROSSING = 0,
					POSITIVE = 1,
					INSIDE = 1
				};
				enum EquationIndex
				{
					A = 0,
					B = 1,
					C = 2,
					D = 3
				};
				void set(const Vector3 & n, float d);
				void calculate(const Vector3 & A, const Vector3 & B, const Vector3 & C);
				void calculate(const Vector3 & normal, const Vector3 & point);
	
				// distance to point
				float 	distance(const Vector3 & point) const;
				int 	classify(const AABBox & aabb) const;
				int 	classify(const Vector3 & point) const;
				bool 	intersect(const Vector3 & start, const Vector3 & direction, Vector3 & hitPoint) const;
				bool 	intersectEdge(const Vector3 & A, const Vector3 & B, Vector3 & hitPoint) const;
				void 	clip(PolygonPtr & poly);
				void 	normalize(void);
	
				float & operator [] (unsigned int i);
	
	
			private:
				Vector3		_normal;
				float		_distance;
				float		_equation[4]; // abcd
		};

	}; // end namespace Moya

}; // end namespace
