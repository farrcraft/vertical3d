/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context3D.h"

#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
    /**
     **/
    Context3D::Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window3D>& window) :
        window_(window) {
        if (!window_ || !window_->instance() || !window_->surface()) {
            throw std::runtime_error("A 3D context needs a created window to render to");
        }
        device_ = boost::make_shared<vulkan::Device>(logger, window_->instance(), window_->surface());
    }

    /**
     **/
    Context3D::~Context3D() {
    }

    /**
     **/
    boost::shared_ptr<vulkan::Device> Context3D::device() const {
        return device_;
    }
};  // namespace v3d::render::realtime
