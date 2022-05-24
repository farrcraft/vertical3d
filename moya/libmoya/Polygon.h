/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "ReyesPrimitive.h"
#include "Vertex.h"
//#include "RenderContext.h"

#include <boost/shared_ptr.hpp>

#include <vector>

namespace v3D
{

	namespace Moya
	{

		class Plane;
		class RenderContext;

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
				virtual v3d::type::AABBox 	bound(void) const;
				virtual void 	split(void);
				virtual bool 	dice(boost::shared_ptr<MicroPolygonGrid> grid, unsigned int grid_size, boost::shared_ptr<RenderContext> rc);

			protected:
				void 			split(const Plane & plane, boost::shared_ptr<Polygon> p1, boost::shared_ptr<Polygon> p2);

			private:
				std::vector<Vertex>		_vertices;
		};
	
	}; // end namespace Moya

}; // end namespace v3D
