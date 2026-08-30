/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Device.h"

#include "Result.h"

#include <cstring>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::render::realtime::vulkan {

    namespace {
        /**
         * The device extensions the renderer cannot do without.
         **/
        const char* const requiredExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };  // namespace

    /**
     **/
    Device::QueueFamilies::QueueFamilies() noexcept :
        graphics(0),
        present(0),
        hasGraphics(false),
        hasPresent(false) {
    }

    /**
     **/
    bool Device::QueueFamilies::complete() const noexcept {
        return hasGraphics && hasPresent;
    }

    /**
     **/
    Device::Device(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Instance>& instance, const boost::shared_ptr<Surface>& surface) :
        instance_(instance),
        surface_(surface),
        logger_(logger),
        physical_(VK_NULL_HANDLE),
        device_(VK_NULL_HANDLE),
        graphicsQueue_(VK_NULL_HANDLE),
        presentQueue_(VK_NULL_HANDLE) {
        selectPhysical();
        createLogical();
    }

    /**
     **/
    Device::~Device() {
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
    }

    /**
     **/
    VkDevice Device::handle() const noexcept {
        return device_;
    }

    /**
     **/
    VkPhysicalDevice Device::physical() const noexcept {
        return physical_;
    }

    /**
     **/
    boost::shared_ptr<Surface> Device::surface() const noexcept {
        return surface_;
    }

    /**
     **/
    const Device::QueueFamilies& Device::families() const noexcept {
        return families_;
    }

    /**
     **/
    VkQueue Device::graphicsQueue() const noexcept {
        return graphicsQueue_;
    }

    /**
     **/
    VkQueue Device::presentQueue() const noexcept {
        return presentQueue_;
    }

    /**
     **/
    Device::QueueFamilies Device::findFamilies(VkPhysicalDevice device) const {
        QueueFamilies families;

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        if (count > 0) {
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
        }

        for (uint32_t index = 0; index < count; index++) {
            if (!families.hasGraphics && (properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                families.graphics = index;
                families.hasGraphics = true;
            }

            if (!families.hasPresent) {
                VkBool32 presentable = VK_FALSE;
                VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface_->handle(), &presentable);
                if (result == VK_SUCCESS && presentable == VK_TRUE) {
                    families.present = index;
                    families.hasPresent = true;
                }
            }

            if (families.complete()) {
                break;
            }
        }

        return families;
    }

    /**
     **/
    bool Device::hasRequiredExtensions(VkPhysicalDevice device) const {
        uint32_t count = 0;
        VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            return false;
        }

        std::vector<VkExtensionProperties> available(count);
        if (count > 0) {
            result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());
            if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
                return false;
            }
        }

        for (const char* extension : requiredExtensions) {
            bool found = false;
            for (const VkExtensionProperties& candidate : available) {
                if (std::strcmp(extension, candidate.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }

        return true;
    }

    /**
     **/
    void Device::selectPhysical() {
        uint32_t count = 0;
        VkResult result = vkEnumeratePhysicalDevices(instance_->handle(), &count, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to count the physical vulkan devices - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
        if (count == 0) {
            throw std::runtime_error("No physical vulkan devices are available");
        }

        std::vector<VkPhysicalDevice> devices(count);
        result = vkEnumeratePhysicalDevices(instance_->handle(), &count, devices.data());
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to enumerate the physical vulkan devices - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        VkPhysicalDeviceProperties selectedProperties{};
        for (VkPhysicalDevice device : devices) {
            if (!hasRequiredExtensions(device)) {
                continue;
            }

            QueueFamilies families = findFamilies(device);
            if (!families.complete()) {
                continue;
            }

            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);

            // a discrete gpu is worth taking over whatever we may have already settled for
            const bool discrete = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            if (physical_ == VK_NULL_HANDLE || discrete) {
                physical_ = device;
                families_ = families;
                selectedProperties = properties;
            }
            if (discrete) {
                break;
            }
        }

        if (physical_ == VK_NULL_HANDLE) {
            throw std::runtime_error("No physical vulkan device can both render and present to the window");
        }

        logger_->get()->info("Using vulkan device {}", std::string(selectedProperties.deviceName));
    }

    /**
     **/
    void Device::createLogical() {
        const float priority = 1.0f;
        std::set<uint32_t> uniqueFamilies;
        uniqueFamilies.insert(families_.graphics);
        uniqueFamilies.insert(families_.present);

        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for (uint32_t family : uniqueFamilies) {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = family;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &priority;
            queueInfos.push_back(queueInfo);
        }

        VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.pEnabledFeatures = &features;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(requiredExtensions));
        createInfo.ppEnabledExtensionNames = requiredExtensions;

        VkResult result = vkCreateDevice(physical_, &createInfo, nullptr, &device_);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create a logical vulkan device - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        vkGetDeviceQueue(device_, families_.graphics, 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, families_.present, 0, &presentQueue_);
    }

};  // namespace v3d::render::realtime::vulkan
