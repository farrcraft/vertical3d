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
#ifndef INCLUDED_V3D_CAMERAPROFILE_H
#define INCLUDED_V3D_CAMERAPROFILE_H

#include <string>

#include <libv3dtypes/Vector3.h>
#include <libv3dtypes/Matrix4.h>
#include <libv3dtypes/Quaternion.h>

namespace v3D
{

	/**
		A class containing the common camera settings.
	 */
	class CameraProfile
	{
		public:
			CameraProfile();
			virtual ~CameraProfile();

			// get
			float		near(void) const;
			float		far(void) const;
			Vector3		eye(void) const;
			bool		orthographic(void) const;
			float		orthoZoom(void) const;
			Vector3		up(void) const;
			Vector3		right(void) const;
			Vector3		direction(void) const;
			float		pixelAspect(void) const;
			Quaternion	rotation(void) const;
			float		fov(void) const;
			std::string	name(void) const;

			// set
			void		near(float n);
			void		far(float f);
			void		orthographic(bool ortho);
			void		eye(const Vector3 & eye);
			void		up(const Vector3 & u);
			void		right(const Vector3 & r);
			void		direction(const Vector3 & d);
			void		pixelAspect(float aspect);
			void		fov(float f);
			void		name(const std::string & name);
			/**
				Set the camera's rotation.
				The rotation tells how to transform the camera into the same space 
				defined by its normals. The normals are the up, right, and direction
				vectors. The default coordinate system with no rotation is a right-
				handed system with right +x, up +y, and direction +z.

				@param q the new rotation value.
			*/
			void		rotation(const Quaternion & q);
			void		orthoZoom(float f);
			/**
				Orient the camera to look at a point in space.
				The camera normals and rotation will be recalculated so the point
				will be in the center of the view.

				@param center the point in world space to focus the camera on.
			 */
			void		lookat(const Vector3 & center);
			/**
				Copy camera settings.

				@param profile the camera profile to copy settings from.
			 */
			void		profile(const CameraProfile & profile);
			/**
				Assignment operator.
				Copy the camera settings from one profile and assign them to another.
				This is just the profile method packaged in an operator.

				@param p the camera profile to copy settings from.
				@return the camera profile with the new settings copied to it.
			 */
			CameraProfile & operator = (const CameraProfile & p);
			/**
				Get the Camera's adaptive projection setting.

				@return the adaptive projection setting.
			 */
			bool adaptiveProjection(void) const;
			/**
				Get the Camera's adaptive position setting.

				@return the adaptive position setting.
			 */
			bool adaptivePosition(void) const;
			/**
				Set the camera's adaptive projection setting.
				In some situations it is necessary to have a z depth beyond the
				bounds of the scene. By specifying an adaptive projection, the
				far camera value will be continuously adjusted. This will prevent
				objects from clipping out as they move farther away.

				@param adaptive the new adaptive projection setting.
			 */
			void adaptiveProjection(bool adaptive);
			/**

				@param adaptive
			 */
			void adaptivePosition(bool adaptive);

		private:
			typedef enum CameraOptions
			{
				OPTION_ORTHOGRAPHIC			= (1 << 1),
				OPTION_ADAPTIVE_PROJECTION	= (1 << 2),
				OPTION_ADAPTIVE_POSITION	= (1 << 3)
			} CameraOptions;

			std::string		_name;
			Vector3			_eye;
			Vector3			_direction;
			Vector3			_right;
			Vector3			_up;
			double			_orthoZoom;
			float			_pixelAspect;	// pixel aspect ratio w:h e.g. 4/3 = 1.33
			float			_near;
			float			_far;
			float			_fov;			// y fov
			unsigned int 	_options;

			Quaternion		_rotation;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_CAMERAPROFILE_H
