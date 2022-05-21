#ifndef INCLUDED_MOYA_FRUSTUM
#define INCLUDED_MOYA_FRUSTUM

#include "Plane.h"

#ifdef WIN32
#include <3dtypes/AABBox.h>
#include <3dtypes/Matrix4.h>
#else
#include <vertical3d/3dtypes/AABBox.h>
#include <vertical3d/3dtypes/Matrix4.h>
#endif

#include <string>
#include <map>

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
			
				enum HitClassification
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
//				int intersect(const Sphere & sphere);
				/*
					clip polygon against frustum - modifies passed polygon structure
					this does a sutherland-hodgeman clip against each plane
					reyes doesn't perform any clipping operations
				*/
				void clip(boost::shared_ptr<Polygon> poly);
	
			private:
				std::map<std::string, Plane> 	_clippingPlanes;
		};

	}; // end namespace Moya

}; // end namespace v3D

#endif // INCLUDED_MOYA_FRUSTUM
