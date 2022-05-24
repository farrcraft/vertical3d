/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../v3dlibs/type/Vector3.h"
#include "../../v3dlibs/type/AABBox.h"

#include "Polygon.h"

namespace v3D
{

	namespace Moya
	{

		/**
		 *	defines a plane from one of:
		 *		N dot P = D
		 *		Ax+Bx+Cz = D
		 */
		class Plane
		{
			public:
				Plane();
				Plane(const v3d::type::Vector3 & A, const v3d::type::Vector3 & B, const v3d::type::Vector3 & C);
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
				void set(const v3d::type::Vector3 & n, float d);
				void calculate(const v3d::type::Vector3 & A, const v3d::type::Vector3 & B, const v3d::type::Vector3 & C);
				void calculate(const v3d::type::Vector3 & normal, const v3d::type::Vector3 & point);
	
				/**
				 * distance to point
				 */
				float 	distance(const v3d::type::Vector3 & point) const;
				int 	classify(const v3d::type::AABBox & aabb) const;
				int 	classify(const v3d::type::Vector3 & point) const;
				bool 	intersect(const v3d::type::Vector3 & start, const v3d::type::Vector3 & direction, v3d::type::Vector3 & hitPoint) const;
				bool 	intersectEdge(const v3d::type::Vector3 & A, const v3d::type::Vector3 & B, v3d::type::Vector3 & hitPoint) const;
				void 	clip(boost::shared_ptr<Polygon> poly);
				void 	normalize(void);
	
				float & operator [] (unsigned int i);
	
	
			private:
				v3d::type::Vector3 _normal;
				float		_distance;
				float		_equation[4]; // abcd
		};

	}; // end namespace Moya

}; // end namespace

