/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MicroPolygonGrid.h"

#include <cassert>
#include <vector>

namespace v3d::moya::reyes {

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
    // the corners in order are [i][j], [i][j+1], [i+1][j+1] and [i+1][j]. A micropolygon is
    // named by its corner with the lowest indices, so i and j each stop one short of size()
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

};  // namespace v3d::moya::reyes
