/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "MicroPolygonGrid.h"

#include <moya/libmoya/Vertex.h>

#include "MicroPolygon.h"

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

Vertex MicroPolygonGrid::vertex(unsigned int column, unsigned int row) const {
    assert(column < size() && row < size());
    return grid_[column][row];
}

MicroPolygon MicroPolygonGrid::microPolygon(unsigned int column, unsigned int row) const {
    // the corners in order are [column][row], [column][row+1], [column+1][row+1] and
    // [column+1][row]. A micropolygon is named by its corner with the lowest indices, so
    // column and row each stop one short of size()
    assert(column + 1 < size() && row + 1 < size());
    MicroPolygon poly;
    poly[0] = grid_[column][row];
    poly[1] = grid_[column][row+1];
    poly[2] = grid_[column+1][row+1];
    poly[3] = grid_[column+1][row];
    return poly;
}

void MicroPolygonGrid::addVertex(const Vertex & vert, unsigned int column, unsigned int row) {
    assert(column < size() && row < size());
    grid_[column][row] = vert;
}

};  // namespace v3d::moya::reyes
