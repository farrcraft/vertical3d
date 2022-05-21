/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include "Vector3.h"
#include "Matrix4.h"
#include "Quaternion.h"

namespace v3d::type
{

	/**
	 * A class containing the common camera settings.
	 */
	class CameraProfile
	{
		public:
			CameraProfile(const std::string & name);
			virtual ~CameraProfile();

			void clipping(float near, float far);
			void eye(const Vector3 & position);

			/**
			*	Orient the camera to look at a point in space.
			*	The camera normals and rotation will be recalculated so the point
			*	will be in the center of the view.
			*	@param center the point in world space to focus the camera on.
			*/
			void lookat(const Vector3 & center);
			/**
			*	Copy camera settings.
			*	@param profile the camera profile to copy settings from.
			*/
			void clone(const CameraProfile & profile);
			/**
			*	Assignment operator.
			*	Copy the camera settings from one profile and assign them to another.
			*	This is just the profile method packaged in an operator.
			*	@param p the camera profile to copy settings from.
			*	@return the camera profile with the new settings copied to it.
			*/
			CameraProfile & operator = (const CameraProfile & p);

		protected:
			friend class Camera;

			typedef enum CameraOptions
			{
				OPTION_ORTHOGRAPHIC			= (1 << 1),
				OPTION_DEFAULT				= (1 << 2)
			} CameraOptions;

			std::string		name_;
			Vector3			eye_;
			Vector3			direction_;
			Vector3			right_;
			Vector3			up_;
			float			orthoZoom_;
			float			pixelAspect_;	// pixel aspect ratio w:h e.g. 4/3 = 1.33
			float			near_;
			float			far_;
			float			fov_;			// y fov
			unsigned int 	options_;
			unsigned int	size_[2];

			Quaternion		rotation_;
	};

}; // end namespace v3D
