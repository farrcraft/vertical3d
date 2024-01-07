/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ReyesPrimitive.h"

namespace v3d::moya {

ReyesPrimitive::ReyesPrimitive() {
}

ReyesPrimitive::~ReyesPrimitive() {
}

bool ReyesPrimitive::diceable(void) const {
    return _diceable;
}

v3d::type::AABBox ReyesPrimitive::bound(void) const {
    return v3d::type::AABBox();
}

void ReyesPrimitive::split(void) {
}

bool ReyesPrimitive::dice(boost::shared_ptr<MicroPolygonGrid> grid) {
    return false;
}

void ReyesPrimitive::diceable(bool status) {
    _diceable = status;
}

};  // namespace v3d::moya
