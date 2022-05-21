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
#include <sstream>
#include <cstdlib>

#include "Vector3.h"

using namespace v3D;


/*
find minimum squared distance between a point and a line with an origin and direction vector.

code based on: "Geometric Tools for Computer Graphics - Philip J. Schneider, David H. Eberly
*/
real_t pointLineDistanceSquared(const Vector3 & point, const Vector3 & origin, const Vector3 & direction, real_t & t)
{
	real_t distance;
	t = direction.dot(point - direction);
	t /= direction.dot(direction);
	Vector3 prime;
	prime = origin + (direction * t);

	Vector3 vec = point - prime;
	distance = vec.dot(vec);

	return distance;
}

real_t v3D::pointLineSegmentDistanceSquared(const Vector3 & point, const Vector3 & seg0, const Vector3 & seg1, real_t & t)
{
	float distance;

	distance = pointLineDistanceSquared(point, seg0, seg1, t);

	if (t < 0.0)
	{
		t = 0.0;
		Vector3 vec = point - seg0;
		distance = vec.dot(vec);
	}
	else if (t > 1.0)
	{
		t = 1.0;
		Vector3 vec = point - seg1;
		distance = vec.dot(vec);
	}
	return distance;
}

// Construction/Destruction
Vector3::Vector3()
{
	_vec[0] = _vec[1] = _vec[2] = 0.;
}

Vector3::Vector3(const Vector3 & v)
{
	_vec[0] = v._vec[0];
	_vec[1] = v._vec[1];
	_vec[2] = v._vec[2];
}

Vector3::Vector3(real_t vec[3])
{
	_vec[0] = vec[0];
	_vec[1] = vec[1];
	_vec[2] = vec[2];
}

Vector3::Vector3(const real_t x, const real_t y, const real_t z)
{
	_vec[0] = x;
	_vec[1] = y;
	_vec[2] = z;
}

Vector3::Vector3(const std::string & vec)
{
	_vec[0] = _vec[1] = _vec[2] = 0.;

	std::string::size_type offset = vec.find(",");
	std::string::size_type length = vec.size();
	if (offset == std::string::npos)
		return;
	std::string vec_x, vec_y, vec_z;
	vec_x = vec.substr(0, offset - 1);
	_vec[0] = atof(vec_x.c_str());
	std::string::size_type start = offset + 1;
	if (start >= length)
		return;
	offset = vec.find(",", start);
	if (offset == std::string::npos)
		return;
	vec_y = vec.substr(start, offset - 1);
	_vec[1] = atof(vec_y.c_str());
	if ((offset + 1) >= length)
		return;
	vec_z = vec.substr((offset + 1), (length - 1));
	_vec[2] = atof(vec_z.c_str());
}

Vector3::~Vector3()
{
}

void Vector3::abs(void)
{
	_vec[0] = fabsf(_vec[0]);
	_vec[1] = fabsf(_vec[1]);
	_vec[2] = fabsf(_vec[2]);
}

void Vector3::clear(void)
{
	_vec[0] = _vec[1] = _vec[2] = 0.0f;
}

void Vector3::clamp(real_t min, real_t max)
{
	vclamp(_vec[0], min, max);
	vclamp(_vec[1], min, max);
	vclamp(_vec[2], min, max);
}

// also known as an inner product
real_t Vector3::dot(const Vector3 & other) const
{
	return ((_vec[0] * other._vec[0]) + (_vec[1] * other._vec[1]) + (_vec[2] * other._vec[2]));
}

real_t Vector3::squaredLength(void) const
{
	return (_vec[0] * _vec[0] + _vec[1] * _vec[1] + _vec[2] * _vec[2]);
}

real_t Vector3::length(void) const
{
	return static_cast<real_t>(std::sqrt(squaredLength()));
}

bool Vector3::isUnit(void) const
{
	return float_eq(1.0f, length());
}

bool Vector3::isZero(void) const
{
	return float_eq(0.0f, length());
}

void Vector3::normalize(void)
{
	real_t r = length();

	if (fabs(r - 1.0f) < EPSILON)
		return;

	r = 1.0f / r;

	*this *= r;
}

real_t Vector3::operator[](unsigned int i) const
{
	assert (i < 3);
	return _vec[i];
}

real_t & Vector3::operator[](unsigned int i)
{
	assert (i < 3);
	return _vec[i];
}

std::string Vector3::str(unsigned int i) const
{
	assert (i < 3);
	std::stringstream buf;
	buf << _vec[i];
	return buf.str();
}

bool Vector3::operator == (const Vector3 & v) const
{
	return  ((_vec[0] == v._vec[0]) &&
			 (_vec[1] == v._vec[1]) &&
			 (_vec[2] == v._vec[2])) ? true : false;
}

bool Vector3::operator != (const Vector3 &v) const
{
	return !(*this == v);
}

Vector3 & Vector3::operator = (const Vector3 & v)
{
	_vec[0] = v._vec[0];
	_vec[1] = v._vec[1];
	_vec[2] = v._vec[2];

	return *this;
}

Vector3 & Vector3::operator += (const Vector3 &v)
{
	_vec[0] += v._vec[0];
	_vec[1] += v._vec[1];
	_vec[2] += v._vec[2];

	return *this;
}

Vector3 & Vector3::operator -= (const Vector3 &v)
{
	_vec[0] -= v._vec[0];
	_vec[1] -= v._vec[1];
	_vec[2] -= v._vec[2];

	return *this;
}

Vector3 & Vector3::operator *= (const real_t f)
{
	_vec[0] *= f;
	_vec[1] *= f;
	_vec[2] *= f;

	return *this;
}

Vector3 & Vector3::operator /= (const real_t f)
{
	assert (f != 0.0f);

	real_t inv = 1.0f / f;

	_vec[0] *= inv;
	_vec[1] *= inv;
	_vec[2] *= inv;

	return *this;
}

Vector3 Vector3::operator - (void) const	// -v1
{
	return Vector3(-_vec[0], -_vec[1], -_vec[2]);
}

Vector3 Vector3::operator + (const Vector3 & v) const	// v1 + v2
{
	return Vector3(_vec[0] + v._vec[0], _vec[1] + v._vec[1], _vec[2] + v._vec[2]);
}

Vector3 Vector3::operator + (const real_t f) const
{
	return Vector3(_vec[0] + f, _vec[1] + f, _vec[2] + f);
}

Vector3 Vector3::operator - (const Vector3 & v) const	// v1 - v2
{
	return Vector3(_vec[0] - v._vec[0], _vec[1] - v._vec[1], _vec[2] - v._vec[2]);
}

Vector3 Vector3::operator * (const real_t f) const	// v1 * scalar
{
	return Vector3(_vec[0] * f, _vec[1] * f, _vec[2] * f);
}

real_t Vector3::operator * (const Vector3 & v) const // v1 * v2 (dot product)
{
	return dot(v);
}

Vector3  Vector3::operator / (const real_t f) const	// v1 / scalar
{
	assert (f != 0.0f);
	real_t inv = 1.0f / f;
	return Vector3(_vec[0] * inv, _vec[1] * inv, _vec[2] * inv);
}


// cross product
Vector3 Vector3::cross(const Vector3 &v) const
{
   return Vector3(
			_vec[1] * v._vec[2] - _vec[2] * v._vec[1],
			_vec[2] * v._vec[0] - _vec[0] * v._vec[2],
			_vec[0] * v._vec[1] - _vec[1] * v._vec[0]);
}

// useful for comparing distances w/o sqrt() for actual distance
real_t Vector3::squaredDistance(const Vector3 & v) const
{
   return 	(_vec[0] - v._vec[0]) * (_vec[0] - v._vec[0]) +
			(_vec[1] - v._vec[1]) * (_vec[1] - v._vec[1]) +
			(_vec[2] - v._vec[2]) * (_vec[2] - v._vec[2]);
}

real_t Vector3::distance(const Vector3 & v) const
{
   return static_cast<real_t>(sqrt(squaredDistance(v)));
}


void Vector3::negate(void)
{
	_vec[0] = -_vec[0];
	_vec[1] = -_vec[1];
	_vec[2] = -_vec[2];
}
