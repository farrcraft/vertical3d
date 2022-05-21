/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "ReyesPrimitive.h"
#include "Vertex.h"

namespace v3D
{

	namespace Moya
	{

		class Plane;

		class Polygon : public ReyesPrimitive
		{
			public:
				Polygon();
				virtual ~Polygon();
	
				void 			addVertex(Vertex vert);
				unsigned int	vertexCount(void) const;
				Vertex			vertex(unsigned int idx) const;
				void			removeVertex(unsigned int idx);
				Vertex &		operator [] (unsigned int idx);
	
				// reyes methods
				//virtual bool 	diceable(void) const;
				virtual AABBox 	bound(void) const;
				virtual void 	split(void);
				virtual bool 	dice(MicroPolygonGridPtr & grid);

			protected:
				void 			split(const Plane & plane, Polygon * p1, Polygon * p2);

			private:
				std::vector<Vertex>		_vertices;
		};
	
		typedef Polygon * PolygonPtr;

	}; // end namespace Moya

}; // end namespace v3D
