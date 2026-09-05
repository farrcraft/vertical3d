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
    class MicroPolygonGrid {
     public:
        /**
         * A grid of size-by-size vertices, which is size-1 by size-1 micropolygons. There is
         * no default constructor because a grid's extent is fixed when it is made and every
         * accessor indexes into it.
         */
        explicit MicroPolygonGrid(unsigned int size);
        ~MicroPolygonGrid();

        /**
         * The number of vertices along one side.
         */
        unsigned int size(void) const;

        Vertex vertex(unsigned int i, unsigned int j) const;
        MicroPolygon microPolygon(unsigned int i, unsigned int j) const;
        void addVertex(const Vertex & vert, unsigned int i, unsigned int j);

     private:
        std::vector< std::vector<Vertex> > _grid;
    };
};  // namespace v3d::moya
