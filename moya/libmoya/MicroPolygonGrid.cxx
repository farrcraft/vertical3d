/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MicroPolygonGrid.h"

#include <cassert>
#include <vector>

namespace v3d::moya {

MicroPolygonGrid::MicroPolygonGrid(unsigned int size) : grid_(size, std::vector<Vertex>(size)) {
}

MicroPolygonGrid::~MicroPolygonGrid() {
}

unsigned int MicroPolygonGrid::size(void) const {
    return static_cast<unsigned int>(grid_.size());
}

Vertex MicroPolygonGrid::vertex(unsigned int i, unsigned int j) const {
    assert(i < size() && j < size());
    return grid_[i][j];
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
    p[0] = grid_[i][j];
    p[1] = grid_[i][j+1];
    p[2] = grid_[i+1][j+1];
    p[3] = grid_[i+1][j];
    return p;
}

void MicroPolygonGrid::addVertex(const Vertex & vert, unsigned int i, unsigned int j) {
    assert(i < size() && j < size());
    grid_[i][j] = vert;
}

};  // namespace v3d::moya
