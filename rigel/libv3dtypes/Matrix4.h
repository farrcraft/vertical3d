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
#ifndef INCLUDED_MOYA_MATRIX4_H
#define INCLUDED_MOYA_MATRIX4_H

#include "3DMath.h"
#include "Vector3.h"

namespace v3D
{

	/*
		RI Spec 3.2 defines a matrix as an array of 16 numbers.
		multidimensional arrays are in row-major order and represented by row vectors.
		various equavalent repesentations:
		[ a, b, c, d ]	[  1,  2,  3,  4 ]	[ 11, 12, 13, 14 ]	[  0,  1,  2,  3 ]	[ 00, 01, 02, 03 ]
		[ e, f, g, h ]	[  5,  6,  7,  8 ]	[ 21, 22, 23, 24 ]	[  4,  5,  6,  7 ]	[ 10, 11, 12, 13 ]
		[ i, j, k, l ]	[  9, 10, 11, 12 ]	[ 31, 32, 33, 34 ]	[  8,  9, 10, 11 ]	[ 20, 21, 22, 23 ]
		[ m, n, o, p ]	[ 13, 14, 15, 16 ]	[ 41, 42, 43, 44 ]	[ 12, 13, 14, 15 ]	[ 30, 31, 32, 33 ]

		Note: OpenGL uses column vectors.
		This means that matrices that rely on this class must be transposed when working with the OpenGL API.
	*/
	class Matrix4
	{
		public:
			Matrix4();
			Matrix4(real_t m[4][4]);
			Matrix4(real_t m[16]);
			Matrix4(const Matrix4 & m);
			Matrix4(real_t a, real_t b, real_t c, real_t d, 
					real_t e, real_t f, real_t g, real_t h,
					real_t i, real_t j, real_t k, real_t l,
					real_t m, real_t n, real_t o, real_t p);
			Matrix4(real_t i);
			~Matrix4();

			void			zero(void);
			void			copy(Matrix4 & m);
			void			identity(void);
			Matrix4			transpose(void);
			const real_t * 	data(void) const;

			real_t			determinant(void);
			void			inverse(void);

			/* All of these are unimplemented:

			void			neg(void);
			void			abs(void);
			void			adjoint(void);

			Matrix4 & operator /= (const real_t);				// m1 / scalar
			const Matrix4 & operator += (const Matrix4 & m);	// m1 += m2
			const Matrix4 & operator -= (const Matrix4 & m);	// m1 -= m2
			Matrix4 operator + (const Matrix4 & m) const;		// m1 + m2
			Matrix4 operator + () const;
			Matrix4 operator - () const;						// unary negate
			Matrix4 operator - (const Matrix4 & m) const;		// m1 - m2
			Matrix4 operator / (const real_t);					// m1 / scalar
			*/

			/*
				affine matrix transformations
			*/
			void		translate(real_t x, real_t y, real_t z);
			void		scale(real_t x, real_t y, real_t z);
			void		rotate(real_t angle, real_t x, real_t y, real_t z);

			Matrix4 & operator=(const Matrix4 &);				// m1 = m2
			const Matrix4 & operator *= (const Matrix4 & m);	// m1 *= m2
			Matrix4 & operator *= (const real_t);				//m1 * scalar
			Matrix4 operator * (const Matrix4 & m) const;		// m1 * m2
			Matrix4 operator * (const real_t);					// m1 * scalar
			Vector3 operator * (const Vector3 & v) const;		// m1 * vector

			bool	operator == (const Matrix4 & m) const;		// m1 == m2
			bool	operator != (const Matrix4 & m) const;		// m1 != m2
			real_t  operator [] (unsigned int i) const;			// get
			real_t &operator [] (unsigned int i);				// set

		private:
			real_t	_mat[16];
	};

	static std::ostream & operator << (std::ostream & out, const Matrix4 & m)
	{
		out << "[" << m[0] << ", " << m[1] << ", " << m[2] << ", " << m[3] << "]" << std::endl;
		out << "[" << m[4] << ", " << m[5] << ", " << m[6] << ", " << m[7] << "]" << std::endl;
		out << "[" << m[8] << ", " << m[9] << ", " << m[10] << ", " << m[11] << "]" << std::endl;
		out << "[" << m[12] << ", " << m[13] << ", " << m[14] << ", " << m[15] << "]" << std::endl;
		return out;
	}

}; // end namespace v3D

#endif // INCLUDED_MOYA_MATRIX4_H
