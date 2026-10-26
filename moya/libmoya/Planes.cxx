/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Planes.h"

#include <api/render/offline/FrameBuffer.h>

#include <limits>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::moya {

boost::shared_ptr<v3d::render::offline::FrameBuffer> Planes::make(unsigned int width, unsigned int height) {
    auto planes = boost::make_shared<v3d::render::offline::FrameBuffer>(width, height, COVERAGE + 1);
    // a pixel nothing hit keeps this depth, and reads as further away than anything drawn
    planes->clear({.plane = DEPTH, .value = std::numeric_limits<float>::max()});
    return planes;
}

};  // namespace v3d::moya
