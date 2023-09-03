/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Device.h"

namespace v3d::input {

    Device::Device(const boost::shared_ptr<entt::dispatcher>& dispatcher) :
        dispatcher_(dispatcher) {
    }

};  // namespace v3d::input
