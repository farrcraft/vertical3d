/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Matrix4.h"
#include "3dtypes.h"
#include "Quaternion.h"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace v3d::type;

// Construction/Destruction

Matrix4::Matrix4()
{
	identity();
}

Matrix4::Matrix4(float m[4][4])
{
	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			matrix_[(i * 4) + j] = m[i][j];
		}
	}
}

Matrix4::Matrix4(float m[16])
{
	std::copy(m, m+16, matrix_);
}

Matrix4::Matrix4(const Matrix4 & m)
{
	std::copy(m.matrix_, m.matrix_+16, matrix_);
}

Matrix4::Matrix4(float a, float b, float c, float d, 
				 float e, float f, float g, float h,
				 float i, float j, float k, float l,
				 float m, float n, float o, float p)
{
	matrix_[0] = a;
	matrix_[1] = b;
	matrix_[2] = c;
	matrix_[3] = d;

	matrix_[4] = e;
	matrix_[5] = f;
	matrix_[6] = g;
	matrix_[7] = h;

	matrix_[8] = i;
	matrix_[9] = j;
	matrix_[10] = k;
	matrix_[11] = l;

	matrix_[12] = m;
	matrix_[13] = n;
	matrix_[14] = o;
	matrix_[15] = p;
}

Matrix4::Matrix4(float i)
{
	for (unsigned int j = 0; j < 16; j++)
	{
		matrix_[j] = i;
	}
}

Matrix4::~Matrix4()
{
}

void Matrix4::identity()
{
	static const float IdentityMatrix[] =
	{ 
		1., 0., 0., 0.,
		0., 1., 0., 0.,
		0., 0., 1., 0.,
		0., 0., 0., 1. 
	};

	std::copy(IdentityMatrix, IdentityMatrix+16, matrix_);
}

void Matrix4::zero()
{
	for (unsigned int i = 0; i < 16; i++)
	{
		matrix_[i] = 0.0f;
	}
}

void Matrix4::clone(const Matrix4 & m)
{
	std::copy(m.matrix_, m.matrix_+16, matrix_);
}

void Matrix4::column(unsigned int col, const Vector4 & vec)
{
	assert(col < 4);

	matrix_[col] = vec[0];
	matrix_[col+4] = vec[1];
	matrix_[col+8] = vec[2];
	matrix_[col+12] = vec[3];
}

void Matrix4::column(unsigned int col, const Vector3 & vec)
{
	assert(col < 4);

	matrix_[col] = vec[0];
	matrix_[col+4] = vec[1];
	matrix_[col+8] = vec[2];
}

void Matrix4::translate(float x, float y, float z)
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

void Matrix4::translate(const Vector3 & distance)
{
	matrix_[0] += matrix_[12] * distance[0];
	matrix_[1] += matrix_[13] * distance[0];
	matrix_[2] += matrix_[14] * distance[0];
	matrix_[3] += matrix_[15] * distance[0];
	matrix_[4] += matrix_[12] * distance[1];
	matrix_[5] += matrix_[13] * distance[1];
	matrix_[6] += matrix_[14] * distance[1];
	matrix_[7] += matrix_[15] * distance[1];
	matrix_[8] += matrix_[12] * distance[2];
	matrix_[9] += matrix_[13] * distance[2];
	matrix_[10] += matrix_[14] * distance[2];
	matrix_[11] += matrix_[15] * distance[2];
}

void Matrix4::scale(float x, float y, float z)
{
	matrix_[0]  *= x;
	matrix_[5]  *= y;
	matrix_[10] *= z;
}

void Matrix4::rotate(float angle, float x, float y, float z)
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

void Matrix4::rotate(float angle, const Vector3 & axis)
{
	float radians = deg2rad(angle);
	float c = cosf(radians);
	float s = sinf(radians);
	float x = axis[0];
	float y = axis[1];
	float z = axis[2];
	float xx = x * x;
	float xy = x * y;
	float xz = x * z;
	float yy = y * y;
	float yz = y * z;
	float zz = z * z;

	Matrix4 matrix;
	matrix[0] = xx * (1.0f - c) + c;
	matrix[1] = xy * (1.0f - c) + (z * s);
	matrix[2] = xz * (1.0f - c) - (y * s);
	matrix[3] = 0.0f;

	matrix[4] = xy * (1.0f - c) - (z * s);
	matrix[5] = yy * (1.0f - c) + c;
	matrix[6] = yz * (1.0f - c) + (x * s);
	matrix[7] = 0.0f;

	matrix[8] = xz * (1.0f - c) + (y * s);
	matrix[9] = yz * (1.0f - c) - (x * s);
	matrix[10] = zz * (1.0f - c) + c;
	matrix[11] = 0.0f;

	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;
	matrix[15] = 1.0f;

	*this = matrix * (*this);
}

void Matrix4::rotateX(float angle)
{
	float c = cosf(deg2rad(angle));
	float s = sinf(deg2rad(angle));
	Matrix4 m = *this;

	matrix_[4] = m[4] * c + m[8] * -s;
	matrix_[5] = m[5] * c + m[9] * -s;
	matrix_[6] = m[6] * c + m[10] * -s;
	matrix_[7] = m[7] * c + m[11] * -s;
	matrix_[8] = m[4] * s + m[8] * c;
	matrix_[9] = m[5] * s + m[9] * c;
	matrix_[10] = m[6] * s + m[10] * c;
	matrix_[11] = m[7] * s + m[11] * c;
}

void Matrix4::rotateY(float angle)
{
	float c = cosf(deg2rad(angle));
	float s = sinf(deg2rad(angle));
	Matrix4 m = *this;

	matrix_[0] = m[0] * c + m[8] * s;
	matrix_[1] = m[1] * c + m[9] * s;
	matrix_[2] = m[2] * c + m[10] * s;
	matrix_[3] = m[3] * c + m[11] * s;
	matrix_[8] = m[0] * -s + m[8] * c;
	matrix_[9] = m[1] * -s + m[9] * c;
	matrix_[10] = m[2] * -s + m[10] * c;
	matrix_[11] = m[3] * -s + m[11] * c;
}

void Matrix4::rotateZ(float angle)
{
	float c = cosf(deg2rad(angle));
	float s = sinf(deg2rad(angle));
	Matrix4 m = *this;

	matrix_[0] = m[0] * c + m[4] * -s;
	matrix_[1] = m[1] * c + m[5] * -s;
	matrix_[2] = m[2] * c + m[6] * -s;
	matrix_[3] = m[3] * c + m[7] * -s;
	matrix_[4] = m[0] * s + m[4] * c;
	matrix_[5] = m[1] * s + m[5] * c;
	matrix_[6] = m[2] * s + m[6] * c;
	matrix_[7] = m[3] * s + m[7] * c;
}

std::string Matrix4::str() const
{
	std::stringstream stream;
	for (unsigned int offset = 0; offset < 16; offset += 4)
	{
		stream << "[" << matrix_[offset] << ", " << matrix_[offset+1] << ", " << matrix_[offset+2] << ", " << matrix_[offset+3] << "]" << std::endl;
	}
	std::string str = stream.str();
	return str;
}

float Matrix4::operator [] (unsigned int i) const
{
	assert(i < 16);
	return matrix_[i];
}

float & Matrix4::operator[](unsigned int i)
{
	assert(i >= 0 && i <= 15);
	return matrix_[i];
}

Matrix4 & Matrix4::operator=(const Matrix4 & m)
{
	std::copy(m.matrix_, m.matrix_+16, matrix_);

	return *this;
}

Matrix4 & Matrix4::operator*=(const float f)
{
	float * dest = matrix_;

	for (unsigned int i = 0; i < 16; i++)
	{
		*(dest++) *= f;
	}

	return *this;
}

const Matrix4 & Matrix4::operator*=(const Matrix4 & m)
{
	*this = (*this * m);

	return *this;
}

// unary negate
Matrix4 Matrix4::operator-() const
{
	const float * src = matrix_;
	float result[16];
	float * dest = result;

	for (unsigned int i = 0; i < 16; i++)
	{
		*(dest++) = - (*(src++));
	}

	return Matrix4(result);
}

void Matrix4::neg()
{
	float *src = matrix_;
	float *dest = matrix_;

	for (unsigned int i = 0; i < 16; i++)
		*(dest++) = - (*(src++));
}

void Matrix4::abs()
{
	float *src = matrix_;
	float *dest = matrix_;

	for (unsigned int i = 0; i < 16; i++)
	{
		*(dest++) = fabs(*(src++));
	}
}

const float * Matrix4::operator*() const
{
	const float *src = matrix_;
	return src;
}

const Matrix4 v3d::type::operator*(const Matrix4 & lhs, const Matrix4 & rhs)
{
	const float * A = *lhs;
	const float * B = *rhs;
	float result[16];
	float * C = result;

	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			*C  = A[j]    * B[0];
			*C += A[j+4]  * B[1];
			*C += A[j+8]  * B[2];
			*C += A[j+12] * B[3];
			C++;
		}
		B += 4;
	}
	return Matrix4(result);
}

bool Matrix4::operator==(const Matrix4 & m) const
{
	const float *src = m.matrix_;
	const float *dest = matrix_;

	for (unsigned int i = 0; i < 16; i++)
	{
		if (*(dest++) != *(src++))
		{
			return false;
		}
	}

	return true;
}

bool Matrix4::operator!=(const Matrix4 & m) const
{
	return (!(*this == m));
}

const Matrix4 v3d::type::operator-(const Matrix4 & lhs, const Matrix4 & rhs)
{
	const float * src1 = *lhs;
	const float * src2 = *rhs;
	float result[16];
	float * dest = result;

	for (unsigned int i = 0; i < 16; i++)
	{
		*(dest++) = *(src1++) - *(src2++);
	}

	return Matrix4(result);
}

const Matrix4 v3d::type::operator*(const Matrix4 & lhs, const float d)
{
	const float * src = *lhs;
	float result[16];
	float * dest = result;

	for (unsigned int i = 0; i < 16; i++)
	{
		*(dest++) = *(src++) * d;
	}

   return Matrix4(result);
}

const Matrix4 v3d::type::operator+(const Matrix4 & lhs, const Matrix4 & rhs)
{
	const float *src1 = *lhs;
	const float *src2 = *rhs;
	float result[16];
	float *dest = result;

	for (unsigned int i = 0; i < 16; i++)
		*(dest++) = *(src1++) + *(src2++);

	return Matrix4(result);
}

const Vector3 v3d::type::operator*(const Matrix4 & lhs, const Vector3 & v)
{
	Vector3 dest;
	/*
	dest[0] = v[0] * lhs[0] + v[1] * lhs[1] + v[2] * lhs[2] + lhs[3];
	dest[1] = v[0] * lhs[4] + v[1] * lhs[5] + v[2] * lhs[6] + lhs[7];
	dest[2] = v[0] * lhs[8] + v[1] * lhs[9] + v[2] * lhs[10] + lhs[11];
	*/
    dest[0] = (v[0] * lhs[0]) + (v[1] * lhs[4]) + (v[2] * lhs[8]);
	dest[1] = (v[0] * lhs[1]) + (v[1] * lhs[5]) + (v[2] * lhs[9]),
	dest[2] = (v[0] * lhs[2]) + (v[1] * lhs[6]) + (v[2] * lhs[10]);

	return dest;
}

static inline float det2x2(float a, float b, float c, float d) 
{
	return((a)*(d)-(b)*(c));
}


static float det3x3(float a1, float a2, float a3,
					 float b1, float b2, float b3,
					 float c1, float c2, float c3)
{
	return( a1*det2x2(b2,b3,c2,c3)-
			b1*det2x2(a2,a3,c2,c3)+
			c1*det2x2(a2,a3,b2,b3)
	);
}

void Matrix4::adjoint()
{
	// a[j][k]
	// a[i] = j + k * N
	float a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;

	a1 = matrix_[0]; // d_m[0][0];
	b1 = matrix_[1]; // d_m[1][0];
	c1 = matrix_[2]; // d_m[2][0];
	d1 = matrix_[3]; // d_m[3][0];
	a2 = matrix_[4]; // d_m[0][1];
	b2 = matrix_[5]; // d_m[1][1];
	c2 = matrix_[6]; // d_m[2][1];
	d2 = matrix_[7]; // d_m[3][1];
	a3 = matrix_[8]; // d_m[0][2];
	b3 = matrix_[9]; // d_m[1][2];
	c3 = matrix_[10]; // d_m[2][2];
	d3 = matrix_[11]; // d_m[3][2];
	a4 = matrix_[12]; // d_m[0][3];
	b4 = matrix_[13]; // d_m[1][3];
	c4 = matrix_[14]; // d_m[2][3];
	d4 = matrix_[15]; // d_m[3][3];

  matrix_[0]  =  det3x3 (b2, b3, b4, c2, c3, c4, d2, d3, d4);
  matrix_[4]  = -det3x3 (a2, a3, a4, c2, c3, c4, d2, d3, d4);
  matrix_[8]  =  det3x3 (a2, a3, a4, b2, b3, b4, d2, d3, d4);
  matrix_[12] = -det3x3 (a2, a3, a4, b2, b3, b4, c2, c3, c4);
  matrix_[1]  = -det3x3 (b1, b3, b4, c1, c3, c4, d1, d3, d4);
  matrix_[5]  =  det3x3 (a1, a3, a4, c1, c3, c4, d1, d3, d4);
  matrix_[9]  = -det3x3 (a1, a3, a4, b1, b3, b4, d1, d3, d4);
  matrix_[13] =  det3x3 (a1, a3, a4, b1, b3, b4, c1, c3, c4);
  matrix_[2]  =  det3x3 (b1, b2, b4, c1, c2, c4, d1, d2, d4);
  matrix_[6]  = -det3x3 (a1, a2, a4, c1, c2, c4, d1, d2, d4);
  matrix_[10] =  det3x3 (a1, a2, a4, b1, b2, b4, d1, d2, d4);
  matrix_[14] = -det3x3 (a1, a2, a4, b1, b2, b4, c1, c2, c4);
  matrix_[3]  = -det3x3 (b1, b2, b3, c1, c2, c3, d1, d2, d3);
  matrix_[7]  =  det3x3 (a1, a2, a3, c1, c2, c3, d1, d2, d3);
  matrix_[11] = -det3x3 (a1, a2, a3, b1, b2, b3, d1, d2, d3);
  matrix_[15] =  det3x3 (a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

/*
find the inverse of a matrix
using gauss-jordan eliminiation with full pivoting
based on numerical recipes
*/
void inv(float a[16])
{
	// initialize bookkeeping variables
	int pivot[4];
	for (unsigned int m = 0; m < 4; m++)
	{
		pivot[m] = 0;
	}

	float big, inv, dummy; //, temp
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
			}
		}

		index_row[i] = row;
		index_col[i] = col;
		if (a[(col*4)+col] == 0.0)
			return;
		inv = 1.0f / a[(col*4)+col];
		a[(col*4)+col] = 1.0;
		for (unsigned int p = 0; p < 4; p++)
		{
			a[(col*4)+p] *= inv;
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

Matrix4 Matrix4::inverse() const
{
	float dest[16];
	std::copy(matrix_, matrix_+16, dest);
	inv(dest);

	return Matrix4(dest);
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
Matrix4 Matrix4::transpose() const
{
	float m[16];
	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			m[(i*4)+j] = matrix_[i+(j*4)];
		}
	}
	return Matrix4(m);
}

float Matrix4::det() const
{
	// a[j][k]
	// a[i] = j + k * N
	float a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;

	a1 = matrix_[0]; //d_m[0][0];
	b1 = matrix_[1]; //d_m[1][0];
	c1 = matrix_[2]; //d_m[2][0];
	d1 = matrix_[3]; //d_m[3][0];
	a2 = matrix_[4]; //d_m[0][1];
	b2 = matrix_[5]; //d_m[1][1];
	c2 = matrix_[6]; //d_m[2][1];
	d2 = matrix_[7]; //d_m[3][1];
	a3 = matrix_[8]; //d_m[0][2];
	b3 = matrix_[9]; //d_m[1][2];
	c3 = matrix_[10]; //d_m[2][2];
	d3 = matrix_[11]; //d_m[3][2];
	a4 = matrix_[12]; //d_m[0][3];
	b4 = matrix_[13]; //d_m[1][3];
	c4 = matrix_[14]; //d_m[2][3];
	d4 = matrix_[15]; //d_m[3][3];
  return(
    a1 * det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4)-
    b1 * det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4)+
    c1 * det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4)-
    d1 * det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4)
  );
}


