/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

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
	
		typedef MicroPolygon * MicroPolygonPtr;

	}; // end namespace Moya

}; // end namespace v3D
