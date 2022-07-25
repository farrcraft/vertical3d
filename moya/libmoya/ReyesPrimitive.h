/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "MicroPolygonGrid.h"

#include "../../api/type/AABBox.h"

#include <boost/shared_ptr.hpp>

namespace v3d::moya {
	class ReyesPrimitive {
		public:
			ReyesPrimitive();
			virtual ~ReyesPrimitive();
	
			virtual bool diceable(void) const;
			virtual v3d::type::AABBox bound(void) const;
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
			virtual bool dice(boost::shared_ptr<MicroPolygonGrid> grid);
			virtual void diceable(bool status);
	
		private:
			bool		_diceable;
	};
};
