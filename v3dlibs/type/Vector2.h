/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#ifndef USE_TEMPLATE_VECTOR

#include <string>

namespace v3d::type
{

	class Vector2
	{
		public:
			Vector2();
			Vector2(float x, float y);
			Vector2(const std::string & val);

			Vector2 & operator += (const Vector2 & v);
			Vector2 & operator -= (const Vector2 & v);
			Vector2 & operator *= (float f);

			Vector2 operator - () const;

			bool operator == (const Vector2 & v);

			float operator[](unsigned int i) const;
			float & operator[](unsigned int i);

		private:
			float vec_[2];
	};

};

#endif 
