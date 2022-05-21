/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "3dtypes.h"
#include "Vector3.h"
#include "Matrix4.h"

#include <string>
#include <ostream>

namespace v3d::type {

	/**
	 * A Quaternion class.
	 * Quaternions represent 3D rotations.
	 */
	class Quaternion {
		public:
			/**
			 * Default Constructor
			 */
			Quaternion();
			/**
			 * Copy Constructor
			 * @param q the quaternion to copy values from.
			 */
			Quaternion(const Quaternion & q);
			Quaternion(float q[4]);
			Quaternion(float x, float y, float z, float w);
			Quaternion(const Vector3 & axis, float angle);
			Quaternion(const Matrix4 & mat);

			~Quaternion();

			/**
			 * Create a quaternion from euler rotation angles.
			 * @param ax the x axis rotation.
			 * @param ay the y axis rotation.
			 * @param az the z axis rotation.
			 */
			void euler(float ax, float ay, float az);
			/**
			 * Normalize the quaternion.
			 * The normalization of a quaternion is the same as for vectors. 
			 * Each component is divided by the magnitude of the quaternion.
			 * The resulting quaternion will have a magnitude of 1.0.
			 */
			void normalize(void);
			/**
			 * Get the conjugate of the quaternion.
			 * The conjugate of a quaternion has the vector portion negated while 
			 * leaving the scalar w portion unchanged. The inverse of a normalized 
			 * quaternion is the same as the conjugate.
			 * @return the conjugate of the quaterion.
			 */
			Quaternion conjugate(void) const; // negate
			/**
			 * Get the magnitude of the quaternion.
			 * The magnitude of the quaternion is the square root of the sum of the
			 * square of each component.
			 * @return the magnitude of the quaterion.
			 */
			float magnitude(void) const;
			/**
			 * Create a rotation matrix derived from the quaternion.
			 * @return a rotation matrix.
			 */
			Matrix4 matrix(void) const;
			/*
				axis/angle -> quaternion -> axis/angle works
				euler -> quaternion -> axis/angle works
				axis/angle -> quaternion -> euler doesn't work
			*/
			/**
			 * Create a quaternion from a rotation axis and angle of rotation.
			 * @param axis the axis to rotate around.
			 * @param angle the angle of rotation.
			 */
			void rotation(const Vector3 & axis, float angle);
			/**
			 * Convert a quaternion to an rotation axis and angle of rotation.
			 * @param axis the address of a Vector3 to store the rotation axis in.
			 * @param angle the address to store the angle of rotation in.
			 */
			void axis(Vector3 & axis, float & angle) const;
			Vector3 euler(void) const; // convert to euler
			Quaternion & operator = (const Quaternion & q);
			const Quaternion & operator *= (const Quaternion & q);
			Quaternion operator * (const Quaternion & q) const;
			float operator[](unsigned int i) const;
			float & operator[](unsigned int i);

			Quaternion operator + (const Quaternion & q2) const;

			std::string		str(unsigned int i) const;

		private:
			float		quat_[4];
	};

	static std::ostream & operator << (std::ostream & out, const Quaternion & q)
	{
		out << "[" << q[0] << ", " << q[1] << ", " << q[2] << ", " << q[3] << "]";
		return out;
	}

};
