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
#include <cmath>
#include <cassert>
#include <algorithm>

#include "Matrix4.h"
#include "Quaternion.h"

using namespace v3D;

Matrix4::Matrix4()
{
}

Matrix4::Matrix4(real_t m[4][4])
{
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			_mat[(i * 4) + j] = m[i][j];
}

Matrix4::Matrix4(real_t m[16])
{
	std::copy(m, m+16, _mat);
}

Matrix4::Matrix4(const Matrix4 & m)
{
	std::copy(m._mat, m._mat+16, _mat);
}

Matrix4::Matrix4(real_t a, real_t b, real_t c, real_t d, 
				 real_t e, real_t f, real_t g, real_t h,
				 real_t i, real_t j, real_t k, real_t l,
				 real_t m, real_t n, real_t o, real_t p)
{
	_mat[0] = a;
	_mat[1] = b;
	_mat[2] = c;
	_mat[3] = d;

	_mat[4] = e;
	_mat[5] = f;
	_mat[6] = g;
	_mat[7] = h;

	_mat[8] = i;
	_mat[9] = j;
	_mat[10] = k;
	_mat[11] = l;

	_mat[12] = m;
	_mat[13] = n;
	_mat[14] = o;
	_mat[15] = p;
}

Matrix4::Matrix4(real_t i)
{
	for (unsigned int j = 0; j < 16; j++)
		_mat[j] = i;
}

Matrix4::~Matrix4()
{
}

void Matrix4::zero(void)
{
	for (unsigned int i = 0; i < 16; i++)
		_mat[i] = 0.;
}

void Matrix4::copy(Matrix4 & m)
{
	std::copy(m._mat, m._mat+16, _mat);
}

void Matrix4::identity(void)
{
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			if (i == j)
				_mat[(i*4)+j] = 1.0;
			else
				_mat[(i*4)+j] = 0.0;
}

/*
 swap rows and columns of matrix
 if the transpose of M = Mt then (Mt)t = M
 for diagonal matrices Dt = D
	[ 0,  1,  2,  3  ]		[ 0, 4, 8,  12 ]
	[ 4,  5,  6,  7  ]		[ 1, 5, 9,  13 ]
	[ 8,  9,  10, 11 ]		[ 2, 6, 10, 14 ]
	[ 12, 13, 14, 15 ]		[ 3, 7, 11, 15 ]
*/
Matrix4 Matrix4::transpose(void)
{
	real_t m[16];
	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			m[(i*4)+j] = _mat[i+(j*4)];
	return Matrix4(m);
}


/*
	[a b]
	[c d] 
	= ad - bc
*/
real_t det2x2(real_t mat[4])
{
	return ((mat[0] * mat[3]) - (mat[1] * mat[2]));
}

/*
	[a b c]		[0 1 2]
	[d e f]		[3 4 5]
	[g h i]		[6 7 8]
*/
real_t det3x3(real_t mat[9])
{
	real_t cofactor_a, cofactor_b, cofactor_c;
	real_t minor_a[4], minor_b[4], minor_c[4];

	minor_a[0] = minor_c[1] = mat[4];
	minor_a[1] = minor_b[1] = mat[5];
	minor_a[2] = minor_c[3] = mat[7];
	minor_a[3] = minor_b[3] = mat[8];
	minor_b[0] = minor_c[0] = mat[3];
	minor_b[2] = minor_c[2] = mat[6];

	cofactor_a = det2x2(minor_a);
	cofactor_b = -det2x2(minor_b);
	cofactor_c = det2x2(minor_c);

	return ((mat[0] * cofactor_a) + (mat[1] * cofactor_b) + (mat[2] * cofactor_c));
}

real_t det4x4(real_t mat[16])
{
	real_t cofactor_a, cofactor_b, cofactor_c, cofactor_d;
	real_t minor_a[9], minor_b[9], minor_c[9], minor_d[9];

	minor_a[0] = minor_c[1] = minor_d[1] = mat[5];
	minor_a[1] = minor_b[1] = minor_d[2] = mat[6];
	minor_a[2] = minor_b[2] = minor_c[2] = mat[7];
	minor_a[3] = minor_c[4] = minor_d[4] = mat[9];
	minor_a[4] = minor_b[4] = minor_d[5] = mat[10];
	minor_a[5] = minor_b[5] = minor_c[5] = mat[11];
	minor_a[6] = minor_c[7] = minor_d[7] = mat[13];
	minor_a[7] = minor_b[7] = minor_d[8] = mat[14];
	minor_a[8] = minor_b[8] = minor_c[8] = mat[15];

	minor_b[0] = minor_c[0] = minor_d[0] = mat[4];
	minor_b[3] = minor_c[3] = minor_d[3] = mat[8];
	minor_b[6] = minor_c[6] = minor_d[6] = mat[12];

	cofactor_a = det3x3(minor_a);
	cofactor_b = -det3x3(minor_b);
	cofactor_c = det3x3(minor_c);
	cofactor_d = -det3x3(minor_d);

	return ((mat[0] * cofactor_a) + (mat[1] * cofactor_b) + (mat[2] * cofactor_c) + (mat[3] * cofactor_d));
}

//#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

/*
find the inverse of a matrix
using gauss-jordan eliminiation with full pivoting
based on numerical recipes
*/
void inv(real_t a[15], real_t b[15])
{
	// initialize bookkeeping variables
	int pivot[4];
	for (unsigned int m = 0; m < 4; m++)
	{
		pivot[m] = 0;
	}

	real_t big, inv, dummy; //, temp
	unsigned int row, col;
	int index_row[4], index_col[4];

	// iterate over each column to be reduced
	for (unsigned int i = 0; i < 4; i++)
	{
		big = 0.0;
		// look for a pivot element
		for (unsigned int j = 0; j < 4; j++)
		{
			if (pivot[j] != 1)
			{
				for (unsigned int k = 0; k < 4; k++)
				{
					if (pivot[k] == 0)
					{
						if (fabs(a[(j*4)+k]) >= big)
						{
							big = fabs(a[(j*4)+k]);
							row = j;
							col = k;
						}
					}
				}
			}
		}
		++pivot[col];

		if (row != col)
		{
			for (unsigned int p = 0; p < 4; p++)
			{
				swap(a[(row*4)+p], a[(col*4)+p]);
				swap(b[(row*4)+p], b[(col*4)+p]);
			}
		}

		index_row[i] = row;
		index_col[i] = col;
		if (a[(col*4)+col] == 0.0)
			return;
		inv = 1.0 / a[(col*4)+col];
		a[(col*4)+col] = 1.0;
		for (unsigned int p = 0; p < 4; p++)
		{
			a[(col*4)+p] *= inv;
			b[(col*4)+p] *= inv;
		}

		// reduce rows
		for (unsigned int p = 0; p < 4; p++)
		{
			if (p != col)
			{
				dummy = a[(p*4)+col];
				a[(p*4)+col] = 0.0;
				for (unsigned int q = 0; q < 4; q++)
				{
					a[(p*4)+q] -= a[(col*4)+q] * dummy;
					b[(p*4)+q] -= b[(col*4)+q] * dummy;
				}
			}
		}
	}
	for (int p = 3; p >= 0; p--)
	{
		if (index_row[p] != index_col[p])
		{
			for (unsigned int k = 0; k < 4; k++)
			{
				swap(a[(k*4)+index_row[p]], a[(k*4)+index_col[p]]);
			}
		}
	}
}

real_t Matrix4::determinant(void)
{
	return det4x4(_mat);
}

void Matrix4::inverse(void)
{
	real_t b[16];
	inv(_mat, b);
}

/*

void Matrix4::neg(void)
{
}

void Matrix4::abs(void)
{
}

void Matrix4::adjoint(void)
{
}

Matrix4 Matrix4::operator / (const real_t)
{
}

// negate operator
Matrix4 Matrix4::operator - () const
{
}

Matrix4 Matrix4::operator - (const Matrix4 & m) const
{
}

Matrix4 Matrix4::operator + (const Matrix4 & m) const
{
}

Matrix4 Matrix4::operator + () const
{
}

const Matrix4 & Matrix4::operator += (const Matrix4 & m)
{
}

const Matrix4 & Matrix4::operator -= (const Matrix4 & m)
{
}

Matrix4 & Matrix4::operator /= (const real_t f)
{
}
*/

const real_t * Matrix4::data(void) const
{
	return _mat;
}

void Matrix4::translate(real_t x, real_t y, real_t z)
{
	/*
		[  0,  1,  2,  3 ]
		[  4,  5,  6,  7 ]
		[  8,  9, 10, 11 ]
		[ 12, 13, 14, 15 ]

		[1	0	0	x]
		[0	1	0	y]
		[0	0	1	z]
		[0	0	0	1]
	*/
	
	Matrix4 m;
	m.identity();
	m[3]  = x;
	m[7]  = y;
	m[11] = z;

	*this *= m;
	
	/*
	_mat[3]  = _mat[0]  * x + _mat[1]  * y + _mat[2]  * z + _mat[3];
	_mat[7]  = _mat[4]  * x + _mat[5]  * y + _mat[6]  * z + _mat[7];
	_mat[11] = _mat[8]  * x + _mat[9]  * y + _mat[10] * z + _mat[11];
	_mat[15] = _mat[12] * x + _mat[13] * y + _mat[14] * z + _mat[15];
	*/
}

void Matrix4::scale(real_t x, real_t y, real_t z)
{
	_mat[0]  *= x;
	_mat[5]  *= y;
	_mat[10] *= z;
}

void Matrix4::rotate(real_t angle, real_t x, real_t y, real_t z)
{
	Quaternion q;
	q.rotation(Vector3(x, y, z), angle);
	*this *= q.matrix();
	/*
		[ (x*x)*(1-c)+c 	(x*y)*(1-c)-zs	(x*z)*(1-c)+(y*s)	0]
		[ (x*y)*(1-c)+zs	(y*y)*(1-c)+c	(y*z)*(1-c)-(x*s)	0]
		[ (x*z)*(1-c)-yz	(y*z)*(1-c)+xs	(z*z)*(1-c)+c		0]
		[0					0				0					1]

		c = cos(angle)
		s = sin(angle)
		(x, y, z) = 1 	(normalized)
	*/
}


// assignment operator
Matrix4 & Matrix4::operator=(const Matrix4 & m)
{
	std::copy(m._mat, m._mat+16, _mat);
}

Vector3 Matrix4::operator * (const Vector3 & v) const
{
	Vector3 dest;
	dest[0] = v[0] * _mat[0] + v[1] * _mat[1] + v[2] * _mat[2] + _mat[3];
	dest[1] = v[0] * _mat[4] + v[1] * _mat[5] + v[2] * _mat[6] + _mat[7];
	dest[2] = v[0] * _mat[8] + v[1] * _mat[9] + v[2] * _mat[10] + _mat[11];

	return dest;
}

/*
	matrix*matrix multiplication (shorthand for M = M * N)
	Note: matrix multiplication is associative, but not communative:
		A * B != B * A, (A*B)*C == A*(B*C)
	Also, the transpose of the product of A and B is the same as product of the transposes in reverse (for any n matrices)
	(A*B)t = Bt * At
	C = A * B

	[ a, b, c, d ]	[  1,  2,  3,  4 ]	[ 11, 12, 13, 14 ]
	[ e, f, g, h ]	[  5,  6,  7,  8 ]	[ 21, 22, 23, 24 ]
	[ i, j, k, l ]	[  9, 10, 11, 12 ]	[ 31, 32, 33, 34 ]
	[ m, n, o, p ]	[ 13, 14, 15, 16 ]	[ 41, 42, 43, 44 ]

	[ 0,  1,  2,  3  ]
	[ 4,  5,  6,  7  ]
	[ 8,  9,  10, 11 ]
	[ 12, 13, 14, 15 ]

	cij = ai * bj
	0   =   0 * 0   + 1   * 4   + 2   * 8   + 3   * 12
	c11 = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41
	1   =   0 * 1   + 1   * 5   + 2   * 9   + 3   * 13
	c12 = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42
	2   =   0 * 2   + 1   * 6   + 2   * 10  + 3   * 14
	c13 = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43
	
	4   =   4 * 0   + 5   * 4   + 6   * 8   + 7   * 12
	c21 = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41
	
	8   =   8 * 0   + 9   * 4   + 10  * 8   + 11  * 12
	c31 = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41
*/
const Matrix4 & Matrix4::operator *= (const Matrix4 & m)
{
	real_t c[16];
	unsigned int idx;
	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			idx = (i * 4) + j; // [0..15]
			c[idx]  = _mat[(i*4)]   * m._mat[j];
			c[idx] += _mat[(i*4)+1] * m._mat[j+4];
			c[idx] += _mat[(i*4)+2] * m._mat[j+8];
			c[idx] += _mat[(i*4)+3] * m._mat[j+12];
		}
	}
std::copy(c, c+16, _mat);
	return *this;
}

// matrix*scalar multiplication (shorthand for M = M * k)
Matrix4 & Matrix4::operator *= (const real_t f)
{
	for (unsigned int i = 0; i < 16; i++)
		_mat[i] = f * _mat[i];
	return *this;
}

// matrix*matrix multiplication
Matrix4 Matrix4::operator * (const Matrix4 & m) const
{
	Matrix4 c(*this);
	c *= m;
	return c;
}

// matrix*scalar multiplication - M * k
// M * k = k * M
Matrix4 Matrix4::operator * (const real_t k)
{
	Matrix4 m(*this);
	m *= k;
	return m;
}

bool Matrix4::operator == (const Matrix4 & m) const
{
	for (unsigned int i = 0; i < 16; i++)
		if (_mat[i] != m._mat[i])
			return false;
	return true;
}

bool Matrix4::operator != (const Matrix4 & m) const
{
	return !(*this == m);
}

real_t Matrix4::operator [] (unsigned int i) const
{
	assert(i < 16);
	return _mat[i];
}

real_t & Matrix4::operator [] (unsigned int i)
{
	assert(i < 16);
	return _mat[i];
}
