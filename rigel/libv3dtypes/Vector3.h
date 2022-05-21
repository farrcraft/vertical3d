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
#ifndef INCLUDED_MOYA_VECTOR3_H
#define INCLUDED_MOYA_VECTOR3_H

#include <string>
#include <ostream>

#include "3DMath.h"

namespace v3D
{

	/**
		A 3D Vector class.
	 */
	class Vector3
	{
		public:
			/**
				Default Constructor
			 */
			Vector3();
			/**
				Copy Constructor
			 */
			Vector3(const Vector3 & v);
			Vector3(real_t v[3]);
			Vector3(real_t x, real_t y, real_t z);
			Vector3(const std::string & vec);
			~Vector3();

			void		abs(void);
			void		clamp(real_t min, real_t max);
			/**
				Reset the components of the vector to 0.0.
			 */
			void		clear(void);
			Vector3 	cross(const Vector3 & v) const;
			real_t		distance(const Vector3 & v) const;
			/**
				Calculate the dot product of two vectors.
				@param v a vector to calculate a dot product with.
				@return the dot product of the two vectors.
			 */
			real_t		dot(const Vector3 & v) const;
			/**
				Calculate the length of the vector.
				@return the length of the vector.
			 */
			real_t		length(void) const;
			/**
				Negate the components of the vector.
			 */
			void		negate(void);
			/**
				Normalize to a unit vector.
			 */
			void		normalize(void);
			real_t		squaredLength(void) const;
			real_t		squaredDistance(const Vector3 &) const;
			bool		isUnit(void) const;
			bool		isZero(void) const;

			real_t 			operator[](unsigned int i) const;
			real_t &		operator[](unsigned int i);

			std::string		str(unsigned int i) const;

			bool			operator == (const Vector3 & v) const;	// equality this == v
			bool			operator != (const Vector3 & v) const; 	// equality this != v
			Vector3 &		operator = (const Vector3 & v);			// assignment this = v
			Vector3 &		operator += (const Vector3 &v);			// this += v
			Vector3 &		operator -= (const Vector3 &v);			// this -= v
			Vector3 &		operator *= (const Vector3 &v);			// this *= v
			Vector3 &		operator *= (const real_t f);			// this *= f
			Vector3 &		operator /= (const real_t f);			// this /= f
			Vector3			operator - (void) const;				// -v1
			Vector3			operator + (const Vector3 & v) const;	// v1 + v2
			Vector3			operator + (const real_t f)	const;		// v1 + scalar
			Vector3			operator - (const Vector3 & v) const;	// v1 - v2
			Vector3			operator * (const real_t f)	const;		// v1 * scalar
			real_t			operator * (const Vector3 & v) const;	// v1 * v2 (dot product)
			Vector3			operator / (const Vector3 &) const;
			Vector3			operator / (const real_t f) const;		// v1 / scalar
			//Vector3			operator ^ (const Vector3 &) const;		// cross product

		private:
			real_t	_vec[3];
	};

	real_t pointLineSegmentDistanceSquared(const Vector3 & point, const Vector3 & seg0, const Vector3 & seg1, real_t & t);

	static std::ostream & operator << (std::ostream & out, const Vector3 & v)
	{
		out << "[" << v[0] << ", " << v[1] << ", " << v[2] << "]";
		return out;
	}

}; // end namespace v3D

#endif // INCLUDED_MOYA_VECTOR3_H
