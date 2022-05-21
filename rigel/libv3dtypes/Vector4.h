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
#ifndef INCLUDED_MOYA_VECTOR4_H
#define INCLUDED_MOYA_VECTOR4_H

#include <iostream>

#include "3DMath.h"

namespace v3D
{

	class Vector4
	{
		public:
			Vector4();
			Vector4(const Vector4 & v);
			Vector4(real_t v[4]);
			Vector4(real_t x, real_t y, real_t z, real_t w);
			~Vector4();

			void		abs(void);
			void		clamp(real_t min, real_t max);
			void		clear(void);
			real_t		distance(const Vector4 & v) const;
			real_t		dot(const Vector4 & v) const;
			real_t		length(void) const;
			Vector4 	negate(void) const;
			Vector4 & 	normalize(void);
			real_t		squaredLength(void) const;
			real_t		squaredDistance(const Vector4 &) const;
			bool		isUnit(void) const;
			bool		isZero(void) const;

			real_t 			operator[](unsigned int i) const;
			real_t &		operator[](unsigned int i);
			/*
			operator real_t * (void); // { return _vec; }
			operator const real_t * (void) const; // { return _vec; }
			*/

			bool			operator == (const Vector4 & v) const;	// equality this == v
			bool			operator != (const Vector4 & v) const; 	// equality this != v
			Vector4 &		operator = (const Vector4 & v);			// assignment this = v
			Vector4 &		operator += (const Vector4 &v);			// this += v
			Vector4 &		operator -= (const Vector4 &v);			// this -= v
			Vector4 &		operator *= (const Vector4 &v);			// this *= v
			Vector4 &		operator *= (const real_t f);			// this *= f
			Vector4 &		operator /= (const real_t f);			// this /= f
			Vector4			operator - (void) const;				// -v1
			Vector4			operator + (const Vector4 & v) const;	// v1 + v2
			Vector4			operator + (const real_t f)	const;		// v1 + scalar
			Vector4			operator - (const Vector4 & v) const;	// v1 - v2
			Vector4			operator * (const real_t f)	const;		// v1 * scalar
			real_t			operator * (const Vector4 & v) const;	// v1 * v2 (dot product)
			Vector4			operator / (const Vector4 &) const;
			Vector4			operator / (const real_t f) const;		// v1 / scalar

		private:
			real_t	_vec[4];
	};

	static std::ostream & operator << (std::ostream & out, const Vector4 & v)
	{
		out << "[" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << "]";
		return out;
	}

}; // end namespace v3D

#endif // INCLUDED_MOYA_VECTOR4_H
