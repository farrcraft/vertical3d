/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Vector2.h"

#ifndef USE_TEMPLATE_VECTOR

#include <cassert>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace v3d::type;

Vector2::Vector2()
{
	vec_[0] = vec_[1] = 0.0f;
}

Vector2::Vector2(float x, float y)
{
	vec_[0] = x;
	vec_[1] = y;
}

Vector2::Vector2(const std::string & val)
{
	vec_[0] = vec_[1] = 0.0f;
	if (!val.empty())
	{
		std::string var_x, var_y;
		size_t pos;
		pos = val.find(",");
		var_x = val.substr(0, pos);
		boost::trim(var_x);
		var_y = val.substr(pos+1, val.length());
		boost::trim(var_y);

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
		}
		catch (boost::bad_lexical_cast &)
		{
		}
	}
}

bool Vector2::operator == (const Vector2 & v)
{
	if (vec_[0] == v.vec_[0] && vec_[1] == v.vec_[1])
		return true;
	return false;
}

Vector2 & Vector2::operator += (const Vector2 & v)
{
	vec_[0] += v.vec_[0];
	vec_[1] += v.vec_[1];

	return *this;
}

Vector2 & Vector2::operator *= (float f)
{
	vec_[0] *= f;
	vec_[1] *= f;

	return *this;
}

Vector2 & Vector2::operator -= (const Vector2 & v)
{
	vec_[0] -= v.vec_[0];
	vec_[1] -= v.vec_[1];

	return *this;
}

Vector2 Vector2::operator - () const
{
	return Vector2(-vec_[0], -vec_[1]);
}

float Vector2::operator[](unsigned int i) const
{
	assert (i < 2);
	return vec_[i];
}

float & Vector2::operator[](unsigned int i)
{
	assert (i < 2);
	return vec_[i];
}

#endif
