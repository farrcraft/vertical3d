#ifndef INCLUDED_MOYA_MICROPOLYGONGRID
#define INCLUDED_MOYA_MICROPOLYGONGRID

#include <vector>

#include "MicroPolygon.h"

namespace v3D
{

	namespace Moya
	{

		// a grid of micropolygons
		// micropolygon vertices are shared
		class MicroPolygonGrid
		{
			public:
				MicroPolygonGrid();
				~MicroPolygonGrid();

				Vertex			vertex(unsigned int i, unsigned int j) const;
				MicroPolygon	microPolygon(unsigned int i, unsigned int j) const;
				void 			addVertex(const Vertex & vert, unsigned int i, unsigned int j);

			private:
				std::vector< std::vector<Vertex> >	_grid;
		};

	}; // end namespace Moya

}; // end namespace v3D

#endif // INCLUDED_MOYA_MICROPOLYGONGRID
