/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cassert>

namespace v3d::3dtype
{

	template<typename elemType>
	class vector2
	{
		public:
			typedef elemType value_type;
			typedef value_type vec_type[2];
			vector2()
			{
				vec_[0] = vec_[1] = static_cast<elemType>(0);
			}
			vector2(const elemType & e1, const elemType e2)
			{
				vec_[0] = e1;
				vec_[1] = e2;
			}
			value_type operator[](unsigned int i) const
			{
				assert (i < 2);
				return vec_[i];
			}
			value_type & operator[](unsigned int i)
			{
				assert (i < 2);
				return vec_[i];
			}
			vector2<elemType> & operator += (const vector2<elemType> & v)
			{
				vec_[0] += v.vec_[0];
				vec_[1] += v.vec_[1];

				return *this;
			}
			vector2<elemType> & operator *= (const elemType & f)
			{
				vec_[0] *= f;
				vec_[1] *= f;

				return *this;
			}

			vector2<elemType> & operator -= (const vector2<elemType> & v)
			{
				vec_[0] -= v.vec_[0];
				vec_[1] -= v.vec_[1];

				return *this;
			}

			vector2<elemType> operator - (void) const
			{
				return vector2<elemType>(-vec_[0], -vec_[1]);
			}

			vec_type vec_;
	};

	typedef vector2<float> Vector2f;
	typedef vector2<int> Vector2i;
	typedef vector2<unsigned int> Vector2ui;

};
