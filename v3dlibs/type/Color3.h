/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::type
{

	/**
	 * A 3 component color class.
	 */
	class Color3
	{
		public:

			/**
			 * Color component enumeration
			 */
			typedef enum
			{
				RED = 0,	/**< red component **/
				GREEN,		/**< green component **/
				BLUE		/**< blue component **/
			} Component;

			Color3();
			Color3(float val);
			Color3(float r, float g, float b);
			
			Color3 & operator = (const Color3 & c);
			Color3 & operator += (const Color3 & c);
			float operator [] (unsigned int i) const;
			float & operator[](unsigned int i);

		private:
			float data_[3];
	};

	const Color3 operator * (const Color3 & lhs, const float f);
	const Color3 operator / (const Color3 & lhs, const float f);

	Color3 operator+(const Color3 & lhs, const Color3 & rhs);
	Color3 operator - (const Color3 & lhs, const Color3 & rhs);

};
