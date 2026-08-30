/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Instance.h"

#include "Result.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::render::realtime::vulkan {

    /**
     **/
    Instance::Instance(const boost::shared_ptr<v3d::log::Logger>& logger, const std::vector<const char*>& extensions) :
        instance_(VK_NULL_HANDLE),
        logger_(logger) {
        requireExtensions(extensions);

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vertical3D";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vertical3D Vulkan Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create vulkan instance - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        logger_->get()->info("Created vulkan instance with {} extension(s)", extensions.size());
    }

    /**
     **/
    Instance::~Instance() {
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
    }

    /**
     **/
    VkInstance Instance::handle() const noexcept {
        return instance_;
    }

    /**
     **/
    void Instance::requireExtensions(const std::vector<const char*>& extensions) const {
        uint32_t count = 0;
        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to count vulkan instance extensions - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        std::vector<VkExtensionProperties> available(count);
        if (count > 0) {
            result = vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());
            if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
                std::stringstream msg;
                msg << "Unable to enumerate vulkan instance extensions - " << resultString(result);
                throw std::runtime_error(msg.str());
            }
        }

        std::stringstream missing;
        size_t missingCount = 0;
        for (const char* extension : extensions) {
            bool found = false;
            for (const VkExtensionProperties& candidate : available) {
                if (std::strcmp(extension, candidate.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (missingCount > 0) {
                    missing << ", ";
                }
                missing << extension;
                missingCount++;
            }
        }

        if (missingCount > 0) {
            std::stringstream msg;
            msg << "The vulkan loader is missing required instance extension(s) - " << missing.str();
            throw std::runtime_error(msg.str());
        }
    }

};  // namespace v3d::render::realtime::vulkan
