#ifndef INCLUDED_MOYA_REYESPRIMITIVE
#define INCLUDED_MOYA_REYESPRIMITIVE

#include "MicroPolygonGrid.h"

#ifdef WIN32
#include <3dtypes/AABBox.h>
#else
#include <vertical3d/3dtypes/AABBox.h>
#endif

#include <boost/shared_ptr.hpp>

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
				virtual bool dice(boost::shared_ptr<MicroPolygonGrid> grid);
				virtual void diceable(bool status);
	
			private:
				bool		_diceable;
		};

	}; // end namespace Moya

}; // end namespace v3D

#endif // INCLUDED_MOYA_REYESPRIMITIVE
