/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

namespace v3d::type {

	/**
	 * A 3D Vector class.
	 */
	class Vector3 {
		public:
			/**
			 * Default Constructor
			 */
			Vector3();
			Vector3(float val);
			Vector3(float x, float y, float z);
			Vector3(const std::string & val);

			Vector3 & operator += (const Vector3 & v);
			Vector3 & operator *= (float f);
			Vector3 & operator /= (const float f); // this /= f

			bool operator == (const Vector3 & v) const;	
			bool operator != (const Vector3 & v) const;
			Vector3 operator - () const;		// -v1
			float operator[](unsigned int i) const;
			float & operator[](unsigned int i);

			const float * operator*() const;
			/**
			 * Reset the components of the vector to 0.0.
			 */
			void reset();

			void set(float x, float y, float z);

			Vector3 cross(const Vector3 & v) const;
			/**
			 * Calculate the dot product of two vectors.
			 * @param v a vector to calculate a dot product with.
			 * @return the dot product of the two vectors.
			 */
			float dot(const Vector3 & v) const;
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

	const Vector3	operator / (const Vector3 & lhs, const float f);		// v1 / scalar
	const Vector3	operator + (const Vector3 & lhs, const Vector3 & rhs);	// v1 + v2
	const Vector3	operator + (const Vector3 & lhs, const float f);		// v1 + scalar
	const Vector3	operator - (const Vector3 & lhs, const Vector3 & rhs);	// v1 - v2
	/*
	inline const Vector3 operator - (const Vector3 & lhs, const Vector3 & rhs) // v1 - v2
	{
		return Vector3(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
	}
	*/
	const Vector3	operator * (const Vector3 & lhs, const float f);		// v1 * scalar
	const float operator * (const Vector3 & lhs, const Vector3 & rhs);		// v1 * v2 (dot product)

};
