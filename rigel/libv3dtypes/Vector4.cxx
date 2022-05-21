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

#include "Vector4.h"

using namespace v3D;

// Construction/Destruction
Vector4::Vector4()
{
	_vec[0] = _vec[1] = _vec[2] = 0.;
	 _vec[3] = 1.;
}

Vector4::Vector4(const Vector4 & v)
{
	_vec[0] = v._vec[0];
	_vec[1] = v._vec[1];
	_vec[2] = v._vec[2];
	_vec[3] = v._vec[3];
}

Vector4::Vector4(real_t vec[4])
{
	_vec[0] = vec[0];
	_vec[1] = vec[1];
	_vec[2] = vec[2];
	_vec[3] = vec[3];
}

Vector4::Vector4(real_t x, real_t y, real_t z, real_t w)
{
	_vec[0] = x; 
	_vec[1] = y;
	_vec[2] = z;
	_vec[3] = w;
}

Vector4::~Vector4()
{
}

void Vector4::abs(void)
{
	_vec[0] = fabsf(_vec[0]);
	_vec[1] = fabsf(_vec[1]);
	_vec[2] = fabsf(_vec[2]);
	_vec[3] = fabsf(_vec[3]);
}

void Vector4::clear(void)
{
	_vec[0] = _vec[1] = _vec[2] = _vec[3] = 0.0f;
}

void Vector4::clamp(real_t min, real_t max)
{
	vclamp(_vec[0], min, max);
	vclamp(_vec[1], min, max);
	vclamp(_vec[2], min, max);
	vclamp(_vec[3], min, max);
}

real_t Vector4::dot(const Vector4 & other) const
{
	return ((_vec[0] * other._vec[0]) + (_vec[1] * other._vec[1]) + (_vec[2] * other._vec[2]) + (_vec[3] * other._vec[3]));
}

real_t Vector4::squaredLength(void) const
{
	return (_vec[0] * _vec[0] + _vec[1] * _vec[1] + _vec[2] * _vec[2] + _vec[3] * _vec[3]);
}

real_t Vector4::length(void) const
{
	return static_cast<real_t>(std::sqrt(squaredLength()));
}

bool Vector4::isUnit(void) const
{
	return float_eq(1.0f, length());
}

bool Vector4::isZero(void) const
{
	return float_eq(0.0f, length());
}

Vector4 & Vector4::normalize(void)
{
	real_t r = length();

	if (fabs(r - 1.0f) < EPSILON)
		return *this;

	r = 1.0f / r;

	*this *= r;
	return *this;
}

real_t Vector4::operator[](unsigned int i) const
{
	assert (i < 4);
	return _vec[i];
}

real_t & Vector4::operator[](unsigned int i)
{
	assert (i < 4);
	return _vec[i];
}

bool Vector4::operator == (const Vector4 & v) const
{
	return  ((_vec[0] == v._vec[0]) && 
			 (_vec[1] == v._vec[1]) && 
			 (_vec[2] == v._vec[2]) &&
			 (_vec[3] == v._vec[3])) ? true : false;
}

bool Vector4::operator != (const Vector4 & v) const
{
	return !(*this == v);
}

Vector4 & Vector4::operator = (const Vector4 & v)
{
	_vec[0] = v._vec[0];
	_vec[1] = v._vec[1];
	_vec[2] = v._vec[2];
	_vec[3] = v._vec[3];

	return *this;
}

Vector4 & Vector4::operator += (const Vector4 & v)
{
	_vec[0] += v._vec[0];
	_vec[1] += v._vec[1];
	_vec[2] += v._vec[2];
	_vec[3] += v._vec[3];

	return *this;
}

Vector4 & Vector4::operator -= (const Vector4 & v)
{
	_vec[0] -= v._vec[0];
	_vec[1] -= v._vec[1];
	_vec[2] -= v._vec[2];
	_vec[3] -= v._vec[3];

	return *this;
}

Vector4 & Vector4::operator *= (const real_t f)
{
	_vec[0] *= f;
	_vec[1] *= f;
	_vec[2] *= f;
	_vec[3] *= f;

	return *this;
}

Vector4 & Vector4::operator /= (const real_t f)
{
	assert (f != 0.0f);

	real_t inv = 1.0f / f;

	_vec[0] *= inv;
	_vec[1] *= inv;
	_vec[2] *= inv;
	_vec[3] *= inv;

	return *this;
}

Vector4 Vector4::operator - (void) const	// -v1
{
	return Vector4(-_vec[0], -_vec[1], -_vec[2], -_vec[3]);
}

Vector4 Vector4::operator + (const Vector4 & v) const	// v1 + v2
{
	return Vector4(_vec[0] + v._vec[0], _vec[1] + v._vec[1], _vec[2] + v._vec[2], _vec[3] + v._vec[3]);
}

Vector4 Vector4::operator + (const real_t f) const
{
	return Vector4(_vec[0] + f, _vec[1] + f, _vec[2] + f, _vec[3] + f);
}

Vector4 Vector4::operator - (const Vector4 & v) const	// v1 - v2
{
	return Vector4(_vec[0] - v._vec[0], _vec[1] - v._vec[1], _vec[2] - v._vec[2], _vec[3] - v._vec[3]);
}

Vector4 Vector4::operator * (const real_t f) const	// v1 * scalar
{
	return Vector4(_vec[0] * f, _vec[1] * f, _vec[2] * f, _vec[3] * f);
}

real_t Vector4::operator * (const Vector4 & v) const // v1 * v2 (dot product)
{
	return dot(v);
}

Vector4  Vector4::operator / (const real_t f) const	// v1 / scalar
{
	assert (f != 0.0f);
	real_t inv = 1.0f / f;
	return Vector4(_vec[0] * inv, _vec[1] * inv, _vec[2] * inv, _vec[3] * inv);
}


// useful for comparing distances w/o sqrt() for actual distance
real_t Vector4::squaredDistance(const Vector4 & v) const
{
   return 	(_vec[0] - v._vec[0]) * (_vec[0] - v._vec[0]) +
			(_vec[1] - v._vec[1]) * (_vec[1] - v._vec[1]) +
			(_vec[2] - v._vec[2]) * (_vec[2] - v._vec[2]) + 
			(_vec[3] - v._vec[3]) * (_vec[3] - v._vec[3]);
}

real_t Vector4::distance(const Vector4 & v) const
{
   return static_cast<real_t>(sqrt(squaredDistance(v)));
}


Vector4 Vector4::negate(void) const
{
	return Vector4(-_vec[0], -_vec[1], -_vec[2], -_vec[3]);
}
