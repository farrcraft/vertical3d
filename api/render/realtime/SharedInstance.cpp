/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SharedInstance.h"

#include <api/log/Logger.h>
#include <api/render/realtime/internal/Loaded.h>
#include <api/render/realtime/vulkan/device/Instance.h>

#include <mutex>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::render::realtime {

boost::shared_ptr<vulkan::device::Instance> sharedInstance(const boost::shared_ptr<v3d::log::Logger>& logger) {
    static std::mutex guard;  // process-wide: guards the one instance every thread shares
    static boost::weak_ptr<vulkan::device::Instance> shared;  // process-wide: SDL loads one Vulkan loader per process, and the instance is made through it
    const std::scoped_lock lock(guard);
    boost::shared_ptr<vulkan::device::Instance> instance = shared.lock();
    if (!instance) {
        const boost::shared_ptr<internal::Loaded> loaded = boost::make_shared<internal::Loaded>(logger);
        // the instance shares the ownership of what holds it, so the loader stays while it does
        instance = boost::shared_ptr<vulkan::device::Instance>(loaded, &loaded->instance);
        shared = instance;
    }
    return instance;
}

};  // namespace v3d::render::realtime
