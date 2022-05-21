/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <libv3dtypes/Color3.h>
#include <libv3dtypes/Vector3.h>

namespace v3D
{
	namespace Moya
	{
		class Vertex
		{
			public:
				Vertex();
				~Vertex();
	
				enum Bits
				{
					HAS_COLOR = (1<<0),
					HAS_NORMAL = (1<<1),
					HAS_TEX_COORD = (1<<2)
				};
	
				Vector3	point(void) const;
				void point(const Vector3 & v);
				
			private:
				Vector3			_point;
				Color3			_color;
				Vector3			_normal;
			/*
				short			_edgeCount;
				short			_polygonCount;
			*/
				unsigned int	_bits;
		};

	}; // end namespace Moya

}; // end namespace v3D
