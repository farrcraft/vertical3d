/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef INCLUDED_V3D_QUATERNION_H
#define INCLUDED_V3D_QUATERNION_H

#include "3DMath.h"
#include "Vector3.h"
#include "Matrix4.h"

namespace v3D
{

	/**
		A Quaternion class.
		Quaternions represent 3D rotations.
	 */
	class Quaternion
	{
		public:
			/**
				Default Constructor
			 */
			Quaternion();
			/**
				Copy Constructor

				@param q the quaternion to copy values from.
			 */
			Quaternion(const Quaternion & q);
			Quaternion(real_t q[4]);
			Quaternion(real_t x, real_t y, real_t z, real_t w);
			Quaternion(const Vector3 & axis, real_t angle);
			Quaternion(const Matrix4 & mat);

			~Quaternion();

			/**
				Create a quaternion from euler rotation angles.

				@param ax the x axis rotation.
				@param ay the y axis rotation.
				@param az the z axis rotation.
			 */
			void euler(real_t ax, real_t ay, real_t az);
			/**
				Normalize the quaternion.
				The normalization of a quaternion is the same as for vectors. 
				Each component is divided by the magnitude of the quaternion.
				The resulting quaternion will have a magnitude of 1.0.
			 */
			void normalize(void);
			/**
				Get the conjugate of the quaternion.
				The conjugate of a quaternion has the vector portion negated while 
				leaving the scalar w portion unchanged. The inverse of a normalized 
				quaternion is the same as the conjugate.

				@return the conjugate of the quaterion.
			 */
			Quaternion conjugate(void) const; // negate
			/**
				Get the magnitude of the quaternion.
				The magnitude of the quaternion is the square root of the sum of the
				square of each component.

				@return the magnitude of the quaterion.
			 */
			real_t magnitude(void) const;
			/**
				Create a rotation matrix derived from the quaternion.

				@return a rotation matrix.
			 */
			Matrix4 matrix(void) const;
			/*
				axis/angle -> quaternion -> axis/angle works
				euler -> quaternion -> axis/angle works
				axis/angle -> quaternion -> euler doesn't work
			*/
			/**
				Create a quaternion from a rotation axis and angle of rotation.

				@param axis the axis to rotate around.
				@param angle the angle of rotation.
			 */
			void rotation(const Vector3 & axis, real_t angle);
			/**
				Convert a quaternion to an rotation axis and angle of rotation.

				@param axis the address of a Vector3 to store the rotation axis in.
				@param angle the address to store the angle of rotation in.
			 */
			void axis(Vector3 & axis, real_t & angle) const;
			Vector3 euler(void) const; // convert to euler
			Quaternion & operator = (const Quaternion & q);
			const Quaternion & operator *= (const Quaternion & q);
			Quaternion operator * (const Quaternion & q) const;
			real_t operator[](unsigned int i) const;
			real_t & operator[](unsigned int i);

			Quaternion operator + (const Quaternion & q2) const;

			std::string		str(unsigned int i) const;

		private:
			real_t		_quat[4];
	};

	static std::ostream & operator << (std::ostream & out, const Quaternion & q)
	{
		out << "[" << q[0] << ", " << q[1] << ", " << q[2] << ", " << q[3] << "]";
		return out;
	}

}; // end namespace v3D

#endif // INCLUDED_V3D_QUATERNION_H
