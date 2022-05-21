/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <libv3dtypes/AABBox.h>

#include "MicroPolygonGrid.h"

namespace v3D
{
	namespace Moya
	{

		class ReyesPrimitive
		{
			public:
				ReyesPrimitive();
				virtual ~ReyesPrimitive();
	
				virtual bool diceable(void) const;
				virtual AABBox bound(void) const;
				virtual void split(void);
				/*
					turn a primitive into a micropolygon grid
					i think this is supposed to return 1 or more grids as necessary
					signature would be:
					bool dice(MicroPolygonGridPtr & grid);
					and it would return false until done, each time creating a new grid.
					so you'd just do:
					while (!primitive_ptr->dice(grid)) { do something with grid }
				*/
				virtual bool dice(MicroPolygonGridPtr & grid);
				virtual void diceable(bool status);
	
			private:
				bool		_diceable;
		};

		typedef ReyesPrimitive * ReyesPrimitivePtr;

	}; // end namespace Moya

}; // end namespace v3D
