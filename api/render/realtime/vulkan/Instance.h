/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "../../../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

    /**
     * The application's connection to the vulkan loader.
     * Owns the underlying VkInstance for the lifetime of the object.
     **/
    class Instance final {
     public:
        /**
         * @param logger
         * @param extensions the instance extensions required by the windowing system
         **/
        Instance(const boost::shared_ptr<v3d::log::Logger>& logger, const std::vector<const char*>& extensions);

        /**
         **/
        ~Instance();

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        /**
         * @return the underlying vulkan instance handle
         **/
        VkInstance handle() const noexcept;

     private:
        /**
         * Check the requested extensions against the ones the loader advertises.
         * @throw std::runtime_error if any of them are unavailable
         **/
        void requireExtensions(const std::vector<const char*>& extensions) const;

        VkInstance instance_;
        boost::shared_ptr<v3d::log::Logger> logger_;
    };

};  // namespace v3d::render::realtime::vulkan
