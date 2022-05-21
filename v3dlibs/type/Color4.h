/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::type
{

	class Color4
	{
		public:
			typedef enum
			{
				RED = 0,
				GREEN,
				BLUE,
				ALPHA
			} Component;

			Color4();
			Color4(float r, float g, float b, float a);
			Color4(float * color);

			float operator [] (unsigned int index) const;

			void scale(float s);

		private:
			float data_[4];
	};


};
