/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Vector3.h"

#include <string>

namespace v3d::type {

	/**
	 * A 3D Vector class.
	 */
	class Vector4 {
		public:
			/**
			 * Default Constructor
			 */
			Vector4();
			Vector4(float val);
			Vector4(float x, float y, float z, float w);
			Vector4(const std::string & val);
			Vector4(const Vector3 & vec);
			Vector4(const Vector3 & vec, float w);

			Vector4 & operator += (const Vector4 & v);
			Vector4 & operator *= (float f);
			Vector4 & operator /= (const float f); // this /= f

			bool operator == (const Vector4 & v) const;	
			bool operator != (const Vector4 & v) const;
			Vector4 operator - () const;		// -v1
			float operator[](unsigned int i) const;
			float & operator[](unsigned int i);

			const float * operator*() const;
			/**
			 * Reset the components of the vector to 0.0.
			 */
			void reset();

			/**
			 * Calculate the dot product of two vectors.
			 * @param v a vector to calculate a dot product with.
			 * @return the dot product of the two vectors.
			 */
			float dot(const Vector4 & v) const;
			/**
			 * Calculate the length of the vector.
			 * @return the length of the vector.
			 */
			float length() const;
			/**
			 * Normalize to a unit vector.
			 */
			void normalize();
			float squaredLength() const;

			bool unit() const;
			bool zero() const;

			/**
			 * Create a string representation.
			 * @return the string representation of the vector.
			 */
			std::string str() const;

		private:
			float vec_[3];
	};

	const Vector4	operator / (const Vector4 & lhs, const float f);		// v1 / scalar
	const Vector4	operator + (const Vector4 & lhs, const Vector4 & rhs);	// v1 + v2
	const Vector4	operator + (const Vector4 & lhs, const float f);		// v1 + scalar
	const Vector4	operator - (const Vector4 & lhs, const Vector4 & rhs);	// v1 - v2

	const Vector4	operator * (const Vector4 & lhs, const float f);		// v1 * scalar
	const float operator * (const Vector4 & lhs, const Vector4 & rhs);		// v1 * v2 (dot product)

};
