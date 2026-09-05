/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MicroPolygonGrid.h"

#include <cassert>
#include <vector>

namespace v3d::moya {

MicroPolygonGrid::MicroPolygonGrid(unsigned int size) : _grid(size, std::vector<Vertex>(size)) {
}

MicroPolygonGrid::~MicroPolygonGrid() {
}

unsigned int MicroPolygonGrid::size(void) const {
    return static_cast<unsigned int>(_grid.size());
}

Vertex MicroPolygonGrid::vertex(unsigned int i, unsigned int j) const {
    assert(i < size() && j < size());
    return _grid[i][j];
}

MicroPolygon MicroPolygonGrid::microPolygon(unsigned int i, unsigned int j) const {
    /*
        poly is composed of points on grid:
        [i][j]
        [i][j+1]
        [i+1][j+1]
        [i+1][j]
    */
    // a micropolygon is named by its lower indexed corner, so the last row and column of
    // vertices close the grid rather than opening a polygon of their own
    assert(i + 1 < size() && j + 1 < size());
    MicroPolygon p;
    p[0] = _grid[i][j];
    p[1] = _grid[i][j+1];
    p[2] = _grid[i+1][j+1];
    p[3] = _grid[i+1][j];
    return p;
}

void MicroPolygonGrid::addVertex(const Vertex & vert, unsigned int i, unsigned int j) {
    assert(i < size() && j < size());
    _grid[i][j] = vert;
}

};  // namespace v3d::moya
