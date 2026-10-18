/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "MicroPolygon.h"

namespace v3d::moya::reyes {
/**
 * A grid of vertices, each shared by the micropolygons around it.
 **/
class MicroPolygonGrid {
 public:
    /**
     * A grid of size-by-size vertices, which is size-1 by size-1 micropolygons. The extent
     * is fixed when the grid is made, and every accessor indexes into it.
     */
    explicit MicroPolygonGrid(unsigned int size);
    ~MicroPolygonGrid();

    /**
     * The number of vertices along one side.
     */
    unsigned int size(void) const;

    Vertex vertex(unsigned int column, unsigned int row) const;
    MicroPolygon microPolygon(unsigned int column, unsigned int row) const;
    void addVertex(const Vertex & vert, unsigned int column, unsigned int row);

 private:
    std::vector< std::vector<Vertex> > grid_;
};
};  // namespace v3d::moya::reyes
