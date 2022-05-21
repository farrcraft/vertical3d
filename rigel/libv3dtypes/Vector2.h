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
#ifndef INCLUDED_V3D_VECTOR2_H
#define INCLUDED_V3D_VECTOR2_H

#include <ostream>

#include "3DMath.h"

namespace v3D
{

	class Vector2
	{
		public:
			Vector2();
			Vector2(const Vector2 &v);
			Vector2(real_t vec[2]);
			Vector2(real_t x, real_t y);

			void		abs 		 (void);
			void		set			 (real_t x, real_t y);
			real_t		length		 (void) const;
			Vector2		negate		 (void) const;
			real_t		distance	 (const Vector2 &) const;
			Vector2 &	normalize	 (void);
			void		clear		 (void);
			void		clamp		 (real_t minval, real_t maxval);
			real_t		squaredLength(void) const;
			real_t		squaredDistance(const Vector2 &) const;
			int			isUnit		 (void) const;
			int			isZero		 (void) const;
			real_t		project(real_t radius) const;

			//operator real_t * (void); // { return _data; }
			//operator const real_t * (void) const; // { return _data; }

			const real_t &	operator [] (unsigned int i) const;
			real_t &		operator [] (unsigned int i);
			bool			operator == (const Vector2 & v) const;
			bool			operator != (const Vector2 &) const;
			Vector2 &		operator  = (const Vector2 & v);
			Vector2 &		operator += (const Vector2 &v);
			Vector2 &		operator -= (const Vector2 &v);
			Vector2 &		operator *= (const Vector2 &v);
			Vector2 &		operator *= (const real_t f);
			Vector2 &		operator /= (const real_t f);
			Vector2 &		operator -= (const real_t f);
			Vector2 &		operator += (const real_t f);
			Vector2			operator -  (void) const;				// -v1
			Vector2			operator +  (const Vector2 & v) const;	// v1 + v2
			Vector2			operator +  (const real_t f)	const;	// v1 + scalar
			Vector2			operator -  (const Vector2 & v) const;	// v1 - v2
			Vector2			operator *  (const real_t f)	const;	// v1 * scalar
			Vector2			operator /  (const Vector2 &) const;
			Vector2			operator /  (const real_t f) const;		// v1 / scalar

		private:
			real_t		_vec[2];
	};

	static std::ostream & operator << (std::ostream & out, const Vector2 & v)
	{
		out << "[" << v[0] << ", " << v[1] << "]";
		return out;
	}

}; // end namespace v3D

#endif // INCLUDED_V3D_VECTOR2_H
