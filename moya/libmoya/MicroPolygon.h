#ifndef INCLUDED_MOYA_MICROPOLYGON
#define INCLUDED_MOYA_MICROPOLYGON

#include "Vertex.h"

namespace v3D
{

	namespace Moya
	{
		// a flat shaded quadrilateral with an area of about 1/4 of a pixel
		class MicroPolygon
		{
			public:
				MicroPolygon();
				~MicroPolygon();
	
				Vertex & operator[] (unsigned int i);
	
			private:
				Vertex		_points[4];
		};
	
	}; // end namespace Moya

}; // end namespace v3D

#endif // INCLUDED_MOYA_MICROPOLYGON
