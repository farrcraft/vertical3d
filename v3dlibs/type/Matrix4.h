/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Vector3.h"
#include "Vector4.h"

#include <string>

namespace v3d::type
{

	/**
	 * A 4x4 Matrix class.
	 * The internal representation of the matrix is stored in row major order
	 */
	class Matrix4
	{
		public:
			/**
			 * Default constructor
			 * Initializes an identity matrix
			 */
			Matrix4();
			Matrix4(float m[4][4]);
			Matrix4(float m[16]);
			Matrix4(const Matrix4 & m);
			Matrix4(float a, float b, float c, float d, 
					float e, float f, float g, float h,
					float i, float j, float k, float l,
					float m, float n, float o, float p);
			Matrix4(float i);
			~Matrix4();

			/**
			 * Set all components of the matrix to 0.
			 */
			void zero();
			/**
			 * Copy the values of another matrix into this matrix.
			 * @param m the matrix to be copied.
			 */
			void clone(const Matrix4 & m);
			/**
			 * Set this matrix to the identity matrix.
			 */
			void identity();
			/**
			 * Transpose the matrix.
			 * @return the transposed copy of this matrix
			 */
			Matrix4 transpose() const;
			/**
			 * Invert the matrix.
			 * @return the inverted copy of this matrix.
			 */
			Matrix4 inverse() const;
			/**
			 * Create a string representation.
			 * @return the string representation of the matrix
			 */
			std::string str() const;
			/**
			 * Set a column in the matrix.
			 */
			void column(unsigned int col, const Vector4 & vec);
			void column(unsigned int col, const Vector3 & vec);

			void	neg();
			void	abs();
			void	adjoint();
			float	det() const;

			/*
				affine matrix transformations
			*/
			void		translate(float x, float y, float z);
			void		scale(float x, float y, float z);
			void		rotate(float angle, float x, float y, float z);

			/**
			 * An alternative that does traditional non-quaternion rotation
			 */
			void		rotate(float angle, const Vector3 & axis);
			void		rotateX(float angle);
			void		rotateY(float angle);
			void		rotateZ(float angle);
			void		translate(const Vector3 & distance);

			Matrix4 & operator=(const Matrix4 &);			// m1 = m2

			const Matrix4 & operator*=(const Matrix4 & m);	// m1 *= m2
			Matrix4 & operator*=(const float);				//m1 * scalar

			Matrix4 operator-() const;						// unary negate

			bool operator==(const Matrix4 & m) const;	// m1 == m2
			bool operator!=(const Matrix4 & m) const;	// m1 != m2

			float operator [] (unsigned int i) const;	// get
			float & operator[](unsigned int);			// set

			/**
			 * Access the individual matrix components.
			 * @return a pointer to the matrix data.
			 */
			const float * operator*() const;

		private:
			float matrix_[16]; 
	};

	const Matrix4 operator*(const Matrix4 & lhs, const Matrix4 & rhs); // m1 * m2
	const Matrix4 operator-(const Matrix4 & lhs, const Matrix4 & rhs); // m1 - m2
	const Matrix4 operator+(const Matrix4 & lhs, const Matrix4 & rhs); // m1 + m2
	const Matrix4 operator*(const Matrix4 & lhs, const float); // m1 * scalar
	const Vector3 operator * (const Matrix4 & lhs, const Vector3 & v);	// m1 * vector

};
