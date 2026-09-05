/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <cassert>

#include "MicroPolygon.h"

namespace v3d::moya {

MicroPolygon::MicroPolygon() {
}

MicroPolygon::~MicroPolygon() {
}

Vertex & MicroPolygon::operator[] (unsigned int i) {
    assert(i < 4);
    return points_[i];
}
};  // namespace v3d::moya
