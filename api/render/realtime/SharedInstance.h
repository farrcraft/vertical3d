/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Instance.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

/**
 * The Vulkan instance every window in the process presents through.
 *
 * The first call loads the Vulkan loader through SDL and makes the instance, with the
 * extensions SDL needs to make a surface for a window. Later calls return the same instance
 * while anything still holds it. The last holder to let go destroys it and unloads the loader,
 * so a surface made against it has to be destroyed first. Every holder does that already,
 * because a surface holds its instance.
 *
 * SDL's video subsystem has to be running.
 *
 * @throw std::runtime_error if the loader cannot be loaded or SDL cannot name the extensions
 **/
boost::shared_ptr<vulkan::device::Instance> sharedInstance(const boost::shared_ptr<v3d::log::Logger>& logger);

};  // namespace v3d::render::realtime
