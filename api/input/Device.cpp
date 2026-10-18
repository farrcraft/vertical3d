/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Device.h"

namespace v3d::input {

Device::Device(const boost::shared_ptr<v3d::event::Context>& context, const boost::shared_ptr<entt::dispatcher>& dispatcher) :
    context_(context),
    dispatcher_(dispatcher) {
}

const boost::shared_ptr<v3d::event::Context>& Device::context() const noexcept {
    return context_;
}

const boost::shared_ptr<entt::dispatcher>& Device::dispatcher() const noexcept {
    return dispatcher_;
}

};  // namespace v3d::input
