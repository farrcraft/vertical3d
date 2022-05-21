/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "String.h"

#include <cassert>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace v3d::type;

glm::vec2 string_to_vec2(const std::string & val)
{
	glm::vec2 vec(0.0f);
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
				vec[0] = boost::lexical_cast<float>(var_x);
			}
			if (!var_y.empty())
			{
				vec[1] = boost::lexical_cast<float>(var_y);
			}
		}
		catch (boost::bad_lexical_cast &)
		{
		}
	}
	return vec;
}
