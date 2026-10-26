/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SharedInstance.h"

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Instance.h>

#include <SDL3/SDL_vulkan.h>

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::render::realtime {

namespace {

/**
 * The Vulkan loader, loaded through SDL while this exists. SDL counts the loads, so a window
 * that loaded it as well keeps it.
 **/
class Loader final {
 public:
    Loader() {
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            throw std::runtime_error("The Vulkan loader could not be loaded: " + std::string(SDL_GetError()));
        }
    }
    ~Loader() {
        SDL_Vulkan_UnloadLibrary();
    }
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;
    Loader(Loader&&) = delete;
    Loader& operator=(Loader&&) = delete;
};

/**
 * The extensions SDL needs to make a surface for a window.
 **/
std::vector<const char*> extensions() {
    uint32_t count = 0;
    const char* const* names = SDL_Vulkan_GetInstanceExtensions(&count);
    if (names == nullptr) {
        throw std::runtime_error("SDL could not name the Vulkan extensions a window needs: " + std::string(SDL_GetError()));
    }
    return std::vector<const char*>(names, names + count);
}

/**
 * The loader and the instance made through it. The instance is declared after the loader, so
 * it is made after the loader is loaded and destroyed before the loader is unloaded.
 **/
struct Loaded final {
    explicit Loaded(const boost::shared_ptr<v3d::log::Logger>& logger) : instance(logger, extensions()) {
    }

    Loader loader;
    vulkan::device::Instance instance;
};

};  // namespace

boost::shared_ptr<vulkan::device::Instance> sharedInstance(const boost::shared_ptr<v3d::log::Logger>& logger) {
    static std::mutex guard;
    static boost::weak_ptr<vulkan::device::Instance> shared;
    const std::scoped_lock lock(guard);
    boost::shared_ptr<vulkan::device::Instance> instance = shared.lock();
    if (!instance) {
        const boost::shared_ptr<Loaded> loaded = boost::make_shared<Loaded>(logger);
        // the instance shares the ownership of what holds it, so the loader stays while it does
        instance = boost::shared_ptr<vulkan::device::Instance>(loaded, &loaded->instance);
        shared = instance;
    }
    return instance;
}

};  // namespace v3d::render::realtime
