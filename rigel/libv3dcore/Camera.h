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
#ifndef INCLUDED_V3D_CAMERA_H
#define INCLUDED_V3D_CAMERA_H

#include "CameraProfile.h"

namespace v3D
{

	/**
		A 3D viewing camera.
	 */
	class Camera : public CameraProfile
	{
		public:
			Camera();
			virtual ~Camera();

			// get
			Matrix4	projection(void) const;
			Matrix4 view(void) const;

			Vector3 unproject(const Vector3 & point, int viewport[4]);
			Vector3 project(const Vector3 & point, int viewport[4]);

			/**
				Create a projection matrix.
				The resulting matrix can be retrieved by calling projection().
				If the camera is set to orthographic then an orthographic projection will be created.
				Otherwise a perspective projection will be used.
			*/
			void	createProjection(void);
			/**
				Create a viewing matrix.
				The resulting matrix can be retrieved by calling view().
				The viewing matrix contains the translation and rotation portion of  the 
				camera transformation.
			 */
			void	createView(void);

			// transformations
			/*
				dolly, truck, pedestal are types of translations with special restrictions
				pan and tilt are types of rotation with restrictions
			*/
			/**
				Pan the camera.
				Move horizontally around a fixed axis (the camera's y axis) - camera 
				rotation & look at position changes but eye position doesn't - look left/right
			*/
			void pan(float angle);
			/**
				Tilt the camera.
				Move vertically around a fixed axis (camera's x axis) - look up/down
			*/
			void tilt(float angle);
			/**
				Dolly the camera.
				Move eye forward or backward along direction of view
				same as pedestal but use direction vector instead of up vector
			*/
			void dolly(float d);
			/**
				Truck the camera.
				Move eye on axis perpendicular to direction of view and up axis - move 
				left/right multiply delta value and right vector to get eye delta - 
				right vector must be normalized - add eye delta to current eye position
			*/
			void truck(float d);
			/**
				Zoom the camera.
				Affects the camera lens to zoom in or out (dolly without moving camera)
			*/
			void zoom(float z);
			/**
				Pedestal the camera.
				Move eye on up axis - move up/down
				same as truck but use up vector instead of right vector
			*/
			void pedestal(float d);

		private:
			Vector3		_lookAt;
			Matrix4		_projection;
			Matrix4		_view;			// viewing transformation
	};

	typedef Camera * CameraPtr;

}; // end namespace v3D

#endif // INCLUDED_V3D_CAMERA_H
