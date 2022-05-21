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

#include "Vector2.h"

using namespace v3D;

// Construction/Destruction
Vector2::Vector2()
{
	_vec[0] = 0.;
	_vec[1] = 0.;
}

Vector2::Vector2(const Vector2 &v)
{
	_vec[0] = v._vec[0];
	_vec[1] = v._vec[1];
}

Vector2::Vector2(real_t vec[2])
{
	_vec[0] = vec[0];
	_vec[1] = vec[1];
}

Vector2::Vector2(real_t x, real_t y)
{
	_vec[0] = x; 
	_vec[1] = y;
}

void Vector2::abs(void)
{
	_vec[0] = fabsf(_vec[0]);
	_vec[1] = fabsf(_vec[1]);
}

void Vector2::set(real_t x, real_t y)
{
	_vec[0] = x;
	_vec[1] = y;
}

void Vector2::clear(void)
{
	_vec[0] = 0.;
	_vec[1] = 0.;
}

void Vector2::clamp(real_t minval, real_t maxval)
{
	vclamp(_vec[0], minval, maxval);
	vclamp(_vec[1], minval, maxval);
}

real_t Vector2::squaredLength(void) const
{
	return (_vec[0] * _vec[0] + _vec[1] * _vec[1]);
}

real_t Vector2::length(void) const
{
	return static_cast<real_t>(std::sqrt(squaredLength()));
}

int Vector2::isUnit(void) const
{
	return( float_eq(1.0f, length() ) );
}

int Vector2::isZero(void) const
{
	return( float_eq(0.0f, length()) );
}

Vector2 & Vector2::normalize(void)
{

	real_t r = length();

	if (fabs(r - 1.0f) < EPSILON)
		return *this;

	r = 1.0f / r;

	*this *= r;
	return *this;
}

const real_t & Vector2::operator[](unsigned int i) const
{
	assert (i < 3);
	return _vec[i];
}

real_t & Vector2::operator[](unsigned int i)
{
	assert(i < 3);
	return _vec[i];
}

// equality operator
bool Vector2::operator == (const Vector2 & v) const
{
	return  ((_vec[0] == v._vec[0]) && (_vec[1] == v._vec[1])) ? true : false;
}

bool Vector2::operator!=(const Vector2 &v) const
{
	return !(*this == v);
}

// assignment operator
Vector2 & Vector2::operator = (const Vector2 & v)
{
	_vec[0] = v._vec[0];
	_vec[1] = v._vec[1];

	return *this;
}

Vector2 & Vector2::operator += (const Vector2 &v)
{
	_vec[0] += v._vec[0];
	_vec[1] += v._vec[1];

	return *this;
}

Vector2 & Vector2::operator -= (const Vector2 &v)
{
	_vec[0] -= v._vec[0];
	_vec[1] -= v._vec[1];

	return *this;
}

Vector2 & Vector2::operator *= (const real_t f)
{
	_vec[0] *= f;
	_vec[1] *= f;

	return *this;
}

Vector2 & Vector2::operator -= (const real_t f)
{
	_vec[0] -= f;
	_vec[1] -= f;

	return *this;
}

Vector2 & Vector2::operator += (const real_t f)
{
	_vec[0] += f;
	_vec[1] += f;

	return *this;
}

Vector2 & Vector2::operator /= (const real_t f)
{
	assert (f != 0.0f);

	real_t inv = 1.0f / f;

	_vec[0] *= inv;
	_vec[1] *= inv;

	return *this;
}

Vector2 Vector2::operator - (void) const	// -v1
{
	return Vector2(-_vec[0], -_vec[1]);
}

Vector2 Vector2::operator + (const Vector2 & v) const	// v1 + v2
{
	return Vector2(_vec[0] + v._vec[0], _vec[1] + v._vec[1]);
}

Vector2 Vector2::operator + (const real_t f) const
{
	return Vector2(_vec[0] + f, _vec[1] + f);
}

Vector2 Vector2::operator - (const Vector2 & v) const	// v1 - v2
{
	return Vector2(_vec[0] - v._vec[0], _vec[1] - v._vec[1]);
}

Vector2 Vector2::operator * (const real_t f) const	// v1 * scalar
{
	return Vector2(_vec[0] * f, _vec[1] * f);
}

Vector2 Vector2::operator / (const real_t f) const	// v1 / scalar
{
	assert (f != 0.0f);
	real_t inv = 1.0f / f;
	return Vector2(_vec[0] * inv, _vec[1] * inv);
}


// useful for comparing distances w/o sqrt() for actual distance
real_t Vector2::squaredDistance(const Vector2 & v) const
{
   return   (_vec[0] - v._vec[0]) * (_vec[0] - v._vec[0]) +
			(_vec[1] - v._vec[1]) * (_vec[1] - v._vec[1]);
}

real_t Vector2::distance(const Vector2 & v) const
{
   return static_cast<real_t>(std::sqrt(squaredDistance(v)));
}

Vector2 Vector2::negate(void) const
{
	return Vector2(-_vec[0], -_vec[1]);
}

// project the vector onto a sphere with radius
// a hyperbolic sheet is used if the point is outside the sphere
// based on trackball's tb_project_to_sphere
real_t Vector2::project(real_t radius) const
{
	real_t d, z;

	d = length();
	if (d < (radius * 0.70710678118654752440)) // inside sphere
	{
		z = sqrt(radius * radius - d * d);
	} 
	else // on hyperbola
	{
		real_t t;
		t = radius / 1.41421356237309504880;
		z = t * t / d;
	}
	return z;
}
