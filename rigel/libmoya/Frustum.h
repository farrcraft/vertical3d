/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <map>

#include <libv3dtypes/AABBox.h>
#include <libv3dtypes/Matrix4.h>

#include "Sphere.h"
#include "Plane.h"

namespace v3D
{

	namespace Moya
	{
		class Frustum
		{
			public:
				Frustum();
				Frustum(const Matrix4 & projection);
				~Frustum();

				typedef enum HitClassification
				{
					OUTSIDE		= -1,
					CROSSING	=  0,
					INSIDE		=  1
				};

				// extract the 6 standard clipping planes from a projection matrix
				void extract(const Matrix4 & projection);

				// normalize all planes in frustum
				void normalize(void);

				int intersect(const AABBox & aabb);
				int intersect(const Sphere & sphere);
				/*
					clip polygon against frustum - modifies passed polygon structure
					this does a sutherland-hodgeman clip against each plane
					reyes doesn't perform any clipping operations
				*/
				void clip(PolygonPtr & poly);

			private:
				std::map<std::string, Plane> 	_clippingPlanes;
		};

	}; // end namespace Moya

}; // end namespace v3D
