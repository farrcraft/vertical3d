/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Vector3.h"
#include "3dtypes.h"

#include <cmath>
#include <cassert>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace v3d::type;

Vector3::Vector3()
{
	vec_[0] = vec_[1] = vec_[2] = 0.0f;
}

Vector3::Vector3(float val)
{
	vec_[0] = vec_[1] = vec_[2] = val;
}

Vector3::Vector3(float x, float y, float z)
{
	vec_[0] = x;
	vec_[1] = y;
	vec_[2] = z;
}

Vector3::Vector3(const std::string & val)
{
	vec_[0] = vec_[1] = vec_[2] = 0.0f;
	if (!val.empty())
	{
		std::string var_x, var_y, var_z;
		size_t pos, pos2;
		pos = val.find(",");
		pos2 = val.find(",", pos+1);
		var_x = val.substr(0, pos);
		boost::trim(var_x);
		var_y = val.substr(pos+1, pos2 - pos - 1);
		boost::trim(var_y);
		var_z = val.substr(pos2+1, val.length() - pos2);
		boost::trim(var_z);
		try
		{
			if (!var_x.empty())
			{
				vec_[0] = boost::lexical_cast<float>(var_x);
			}
			if (!var_y.empty())
			{
				vec_[1] = boost::lexical_cast<float>(var_y);
			}
			if (!var_z.empty())
			{
				vec_[2] = boost::lexical_cast<float>(var_z);
			}
		}
		catch (boost::bad_lexical_cast &)
		{
		}
	}
}

void Vector3::set(float x, float y, float z)
{
	vec_[0] = x;
	vec_[1] = y;
	vec_[2] = z;
}

bool Vector3::operator == (const Vector3 & v) const
{
	return  ((vec_[0] == v.vec_[0]) && 
			 (vec_[1] == v.vec_[1]) && 
			 (vec_[2] == v.vec_[2])) ? true : false;
}

bool Vector3::operator != (const Vector3 &v) const
{
	return !(*this == v);
}

Vector3 & Vector3::operator += (const Vector3 & v)
{
	vec_[0] += v.vec_[0];
	vec_[1] += v.vec_[1];
	vec_[2] += v.vec_[2];

	return *this;
}

Vector3 & Vector3::operator *= (float f)
{
	vec_[0] *= f;
	vec_[1] *= f;
	vec_[2] *= f;

	return *this;
}

Vector3 & Vector3::operator /= (const float f)
{
	assert (f != 0.0f);

	float inv = 1.0f / f;

	vec_[0] *= inv;
	vec_[1] *= inv;
	vec_[2] *= inv;

	return *this;
}

Vector3 Vector3::operator - () const	// -v1
{
	return Vector3(-vec_[0], -vec_[1], -vec_[2]);
}

float Vector3::operator[](unsigned int i) const
{
	assert (i < 3);
	return vec_[i];
}

float & Vector3::operator[](unsigned int i)
{
	assert (i < 3);
	return vec_[i];
}

void Vector3::reset(void)
{
	vec_[0] = vec_[1] = vec_[2] = 0.0f;
}

// also known as an inner product
float Vector3::dot(const Vector3 & other) const
{
	return ((vec_[0] * other.vec_[0]) + (vec_[1] * other.vec_[1]) + (vec_[2] * other.vec_[2]));
}

float Vector3::squaredLength(void) const
{
	return (vec_[0] * vec_[0] + vec_[1] * vec_[1] + vec_[2] * vec_[2]);
}

float Vector3::length() const
{
	return static_cast<float>(std::sqrt(squaredLength()));
}

void Vector3::normalize()
{
	float r = length();

	if (fabs(r - 1.0f) < EPSILON)
		return;

	r = 1.0f / r;

	*this *= r;
}

bool Vector3::unit() const
{
	return float_eq(1.0f, length());
}

bool Vector3::zero() const
{
	return float_eq(0.0f, length());
}

// cross product
Vector3 Vector3::cross(const Vector3 & v) const
{
   return Vector3(
			vec_[1] * v.vec_[2] - vec_[2] * v.vec_[1],
			vec_[2] * v.vec_[0] - vec_[0] * v.vec_[2],
			vec_[0] * v.vec_[1] - vec_[1] * v.vec_[0]);
}

const float * Vector3::operator*() const
{
	const float * f = vec_;
	return f;
}

std::string Vector3::str() const
{
	std::stringstream stream;
	stream << vec_[0] << ", " << vec_[1] << ", " << vec_[2];
	std::string str = stream.str();
	return str;
}

const Vector3 v3d::types::operator + (const Vector3 & lhs, const Vector3 & rhs) // v1 + v2
{
	return Vector3(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}

const Vector3 v3d::types::operator + (const Vector3 & lhs, const float f)
{
	return Vector3(lhs[0] + f, lhs[1] + f, lhs[2] + f);
}

const Vector3 v3d::types::operator - (const Vector3 & lhs, const Vector3 & rhs) // v1 - v2
{
	return Vector3(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}

const float v3d::types::operator * (const Vector3 & lhs, const Vector3 & rhs) // v1 * v2 (dot product)
{
	return lhs.dot(rhs);
}

const Vector3 v3d::types::operator / (const Vector3 & lhs, const float f) // v1 / scalar
{
	assert (f != 0.0f);
	float inv = 1.0f / f;
	return Vector3(lhs[0] * inv, lhs[1] * inv, lhs[2] * inv);
}

const Vector3 v3d::types::operator * (const Vector3 & lhs, float f)
{
	Vector3 vec;
	vec[0] = lhs[0] * f;
	vec[1] = lhs[1] * f;
	vec[2] = lhs[2] * f;

	return vec;
}


