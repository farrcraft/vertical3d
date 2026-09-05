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
    return diceable_;
}

v3d::type::AABBox ReyesPrimitive::bound(void) const {
    return v3d::type::AABBox();
}

void ReyesPrimitive::split(RenderContext & rc) {
}

bool ReyesPrimitive::dice(boost::shared_ptr<MicroPolygonGrid> grid, RenderContext & rc) {
    return false;
}

void ReyesPrimitive::diceable(bool status) {
    diceable_ = status;
}

};  // namespace v3d::moya
