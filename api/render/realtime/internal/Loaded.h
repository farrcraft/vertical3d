/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Instance.h>

#include "Loader.h"

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::internal {

/**
 * The loader and the instance made through it. The instance is declared after the loader, so
 * it is made after the loader is loaded and destroyed before the loader is unloaded.
 **/
struct Loaded final {
    explicit Loaded(const boost::shared_ptr<v3d::log::Logger>& logger);

    Loader loader;
    vulkan::device::Instance instance;

 private:
    /**
     * The extensions SDL needs to make a surface for a window.
     **/
    static std::vector<const char*> extensions();
};

};  // namespace v3d::render::realtime::internal
