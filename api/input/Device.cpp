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

};  // namespace v3d::input
