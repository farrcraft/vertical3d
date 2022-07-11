/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "MicroPolygon.h"

namespace v3d::moya {
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
};
