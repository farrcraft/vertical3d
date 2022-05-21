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
#ifndef INCLUDED_V3D_TRANSFORM_MANIPULATOR_H
#define INCLUDED_V3D_TRANSFORM_MANIPULATOR_H

#include <libv3dtypes/Vector2.h>

namespace v3D
{

	extern const unsigned int SELECT_NAME_OBJECT_LIMIT;
	extern const unsigned int SELECT_NAME_MANIPULATOR_LIMIT;

	extern const unsigned int SELECT_NAME_X_MANIPULATOR;
	extern const unsigned int SELECT_NAME_Y_MANIPULATOR;
	extern const unsigned int SELECT_NAME_Z_MANIPULATOR;

	class TransformManipulator
	{
		public:
			TransformManipulator();
			virtual ~TransformManipulator();

			typedef enum AxisConstraints
			{
				CONSTRAINT_NONE,
				CONSTRAINT_X_AXIS,
				CONSTRAINT_Y_AXIS,
				CONSTRAINT_Z_AXIS
			} AxisConstraints;

			typedef enum CoordinateSpace
			{
				COORDINATE_SPACE_GLOBAL,
				COORDINATE_SPACE_LOCAL
			} CoordinateSpace;

			void axis(AxisConstraints constraint);
			AxisConstraints axis(void) const;

			/*
				manipulators work in either local object space or global world space.
				e.g. translate manipulator axis are aligned with world or object coordinate systems.
			*/
			void coordinateSpace(CoordinateSpace space);
			CoordinateSpace coordinateSpace(void) const;

			virtual void draw(void) = 0;
			virtual void transform(const Vector2 & delta) = 0;

		private:
			AxisConstraints			_axisConstraint;
			CoordinateSpace			_coordinateSpace;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_TRANSFORM_MANIPULATOR_H
